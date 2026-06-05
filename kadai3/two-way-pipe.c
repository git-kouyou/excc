#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256
int main(int argc, char *argv[]){
    char buf[BUFSIZE];
    int pipToParent[2], pipToChild[2];
    int pid, msglen, status;
    char *msgToParent = argv[1];
    char *msgToChild = argv[2];
    if (argc != 3) {
        printf("bad argument.\n");
        exit(1);
    }
    if (pipe(pipToParent) == -1) {
        perror("pipe failed.");
        exit(1);
    }
    if (pipe(pipToChild) == -1) {
        perror("pipe failed.");
        exit(1);
    }
    if ((pid=fork())== -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) { /* Child process */
        close(pipToParent[0]);
        close(pipToChild[1]);
        msglen = strlen(msgToChild) + 1;
        if (write(pipToParent[1], msgToParent, msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        if (read(pipToChild[0], buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        printf("Message from parent process: %s\n", buf);
        exit(0);
    } else { /* Parent process */
        close(pipToParent[1]);
        close(pipToChild[0]);
        msglen = strlen(msgToChild) + 1;
        if (write(pipToChild[1], msgToChild, msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        if (read(pipToParent[0], buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        printf("Message from child process: %s\n", buf);
        wait(&status);
    }
}