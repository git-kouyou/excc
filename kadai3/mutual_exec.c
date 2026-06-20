#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int toParent[2];
    int toChild[2];
    pipe(toParent);
    pipe(toChild);
    int buf = 0;
    int max_num = 10;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        exit(1);
    }
    max_num = atoi(argv[1]);

    int pid = fork();
    if(pid < 0) {
        perror("fork");
        exit(1);
    }

    if(pid == 0) {
        close(toParent[0]);
        close(toChild[1]);
        write(toParent[1], &buf, sizeof(buf));
        for(int i = 1; i <= max_num; i++) {
            read(toChild[0], &buf, sizeof(buf));
            printf("Child: %d\n", i);
            write(toParent[1], &buf, sizeof(buf));
        }
        exit(0);
    } else {
        close(toParent[1]);
        close(toChild[0]);
        for(int i = 1; i <= max_num; i++) {
            read(toParent[0], &buf, sizeof(buf));
            printf("Parent: %d\n", i);
            write(toChild[1], &buf, sizeof(buf));
        }
        int status;
        wait(&status);
    }
}