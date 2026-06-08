#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/sem.h>

#define MAX_NUM 100

int main() {
    key_t key;
    int sid;

    if ((key = ftok(".", 1)) == -1) {
        fprintf(stderr, "ftok failed.\n");
        exit(1);
    }

    if ((sid = semget(key, 1, 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "semget failed.\n");
        exit(1);
    }
}