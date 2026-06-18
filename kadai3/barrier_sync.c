#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdbool.h>

#define PROCESS_NUM 5

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int main() {
    int process[PROCESS_NUM];
    int sid;
    union semun arg = {.val = PROCESS_NUM};

    //セマフォの取得
    if ((sid = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT)) == -1){
        perror("semget failed.");
        exit(1);
    }

    if(semctl(sid, 0, SETVAL, arg) == -1) {
        perror("semctl failed.");
        exit(1);
    }

    for (int i = 0; i < PROCESS_NUM; i++) {
        int pid = fork();

        if (pid == -1) {
            perror("fork failed.");
            exit(1);
        } 
        
        if (pid == 0) { //子プロセス
            unsigned int seed = getpid();
            sleep(rand_r(&seed) % 5 + 1);
            printf("Process %d is waiting at the barrier.\n", i);
            //セマフォの値を1減らす
            if(semop(sid, &(struct sembuf){.sem_num = 0, .sem_op = -1, .sem_flg = 0}, 1) == -1) {
                perror("semop failed.");
                exit(1);
            }
            //セマフォの待機
            if(semop(sid, &(struct sembuf){.sem_num = 0, .sem_op = 0, .sem_flg = 0}, 1) == -1) {
                perror("semop failed.");
                exit(1);
            }
            printf("Process %d resumed.\n", i);
            exit(0);
        }

        process[i] = pid;
    }

    printf("Main process is wating at the barrier.\n");
    if(semop(sid, &(struct sembuf){.sem_num = 0, .sem_op = 0, .sem_flg = 0}, 1) == -1) {
        perror("semop failed.");
        exit(1);
    }
    printf("main process resumed.\n");

    for (int i = 0; i < PROCESS_NUM; i++) {
        int status;
        wait(&status);
    }

    if(semctl(sid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed");
        exit(1);
    }
}
