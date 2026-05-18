#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
int main() {
    int i, sid;
    key_t key;
    struct sembuf sb;
    setbuf(stdout, NULL); /* set stdout to be unbufferd */
    if ((key = ftok(".", 1)) == -1){
        fprintf(stderr, "ftok path does not exist.\n");
        exit(1);
    }

    if ((sid=semget(key, 1, 0666 | IPC_CREAT)) == -1) {
        perror("semget error.");
        exit(1);
    }
    printf("waiting for semaphore=1.\n");
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("sem_wait semop error.");
        exit(1);
    }
    printf("done.\n");
    exit(0);
}