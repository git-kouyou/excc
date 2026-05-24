#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUMPROCS 4

char filename[] = "counter";

int count1(int sid) {
    FILE *ct;
    int count;
    struct sembuf sb;

    //セマフォのロック
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("sem_wait semop error.");
        exit(1);
    }

    if ((ct=fopen(filename, "r"))==NULL) exit(1);
    fscanf(ct, "%d\n", &count);
    count++;
    fclose(ct);
    if ((ct=fopen(filename, "w"))==NULL) exit(1);
    fprintf(ct, "%d\n", count);
    fclose(ct);

    sb.sem_op = 1;
    if (semop(sid, &sb, 1) == -1) {
        perror("sem_wait semop error.");
        exit(1);
    }

    return count;
}

int main() {
    int i, count, pid, sid, status;
    FILE *ct;
    key_t key;

    setbuf(stdout, NULL); /* set stdout to be unbufferd */
    count = 0;
    if ((ct=fopen(filename, "w"))==NULL) exit(1);
    fprintf(ct, "%d\n", count);
    fclose(ct);

    //セマフォの取得
    if ((key = ftok(filename, 1)) == -1){
        fprintf(stderr, "ftok failed.\n");
        exit(1);
    }
    if ((sid = semget(key, 1, 0666 | IPC_CREAT)) == -1) {
        perror("semget failed.");
        exit(1);
    }

    if(semctl(sid, 0, SETVAL, 1) == -1) {
        perror("semctl SETVAL failed.");
        exit(1);
    }

    for (i=0; i<NUMPROCS; i++) {
        if ((pid=fork())== -1) {
            perror("fork failed.");
            exit(1);
        }
        if (pid == 0) { /* Child process */
            count = count1(sid);
            printf("count = %d\n", count);
            exit(0);
        }
    }

    for (i=0; i<NUMPROCS; i++) {
        wait(&status);
    }

    //セマフォの削除
    if (semctl(sid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed");
    }

    exit(0);
}