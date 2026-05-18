#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256
int main(int argc, char *argv[]){
    char buf[BUFSIZE];
    int fd[2];
    int pid, msglen, status;
    if (argc != 2) {
        printf("bad argument.\n");
        exit(1);
    }
    if (pipe(fd) == -1) {
        perror("pipe failed.");
        exit(1);
    }
    if ((pid=fork())== -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) { /* Child process */
        close(fd[0]);
        msglen = strlen(argv[1]) + 1;
        if (write(fd[1], argv[1], msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        exit(0);
    } else { /* Parent process */
        close(fd[1]);
        if (read(fd[0], buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        printf("Message from child process: \n");
        printf("\t%s\n",buf);
        wait(&status);
    }
}