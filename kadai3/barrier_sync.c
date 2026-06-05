#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdbool.h>

#define PROCESS_NUM 5

bool interrupted = false;

union semun {
    int              val;    /* SETVAL コマンド用の値 */
    struct semid_ds *buf;    /* IPC_STAT, IPC_SET 用のバッファ */
    unsigned short  *array;  /* GETALL, SETALL 用の配列 */
    struct seminfo  *__buf;  /* IPC_INFO 用のバッファ (Linux固有) */
};

int main() {
    int process[PROCESS_NUM];
    key_t key;
    int sid;
    union semun arg = {.val = PROCESS_NUM};

    if ((key = ftok(".", 1)) == -1) {
        fprintf(stderr, "ftok failed.\n");
        exit(1);
    }

    //セマフォの取得
    if ((sid = semget(key, 1, 0666 | IPC_CREAT)) == -1) {
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
        
        if (pid == 0) { /* Child process */
            sleep(i % 3 + 2);
            semop(sid, &(struct sembuf){.sem_num = 0, .sem_op = -1, .sem_flg = 0}, 1);
            printf("Process %d is waiting at the barrier.\n", i);
            semop(sid, &(struct sembuf){.sem_num = 0, .sem_op = 0, .sem_flg = 0}, 1);
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

    if(semctl(sid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed");
        exit(1);
    }

    for (int i = 0; i < PROCESS_NUM; i++) {
        int status;
        wait(&status);
    }
}
