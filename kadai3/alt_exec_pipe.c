#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_NUM 100

int main() {
    int toParent[2];
    int toChild[2];
    pipe(toParent);
    pipe(toChild);
    int buf = 0;

    int pid = fork();
    if(pid < 0) {
        perror("fork");
        exit(1);
    }

    if(pid == 0) {
        close(toParent[0]);
        close(toChild[1]);
        write(toParent[1], &buf, sizeof(buf));
        for(int i = 1; i <= MAX_NUM; i++) {
            read(toChild[0], &buf, sizeof(buf));
            printf("Child: %d\n", i);
            write(toParent[1], &buf, sizeof(buf));
        }
        exit(0);
    } else {
        close(toParent[1]);
        close(toChild[0]);
        for(int i = 1; i <= MAX_NUM; i++) {
            read(toParent[0], &buf, sizeof(buf));
            printf("Parent: %d\n", i);
            write(toChild[1], &buf, sizeof(buf));
        }
        int status;
        wait(&status);
    }
}