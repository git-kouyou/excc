#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256
int main(int argc, char *argv[]){
    char buf[BUFSIZE];
    int pipeToParent[2], pipeToChild[2];
    int pid, msglen, status;
    char *msgToParent = argv[1];
    char *msgToChild = argv[2];
    if (argc != 3) {
        printf("bad argument.\n");
        exit(1);
    }
    if (pipe(pipeToParent) == -1) {
        perror("pipe failed.");
        exit(1);
    }
    if (pipe(pipeToChild) == -1) {
        perror("pipe failed.");
        exit(1);
    }
    if ((pid=fork())== -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) { /* Child process */
        close(pipeToParent[0]);
        close(pipeToChild[1]);
        msglen = strlen(msgToChild) + 1;
        if (write(pipeToParent[1], msgToParent, msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        if (read(pipeToChild[0], buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        printf("Message from parent process: %s\n", buf);
        exit(0);
    } else { /* Parent process */
        close(pipeToParent[1]);
        close(pipeToChild[0]);
        msglen = strlen(msgToChild) + 1;
        if (write(pipeToChild[1], msgToChild, msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        if (read(pipeToParent[0], buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        printf("Message from child process: %s\n", buf);
        wait(&status);
    }
}