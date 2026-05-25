#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUMPROCS 3
int main() {
    int i, pid, status;
    int children[NUMPROCS];
    for (i=0; i < NUMPROCS; i++) {
        if ((pid = fork()) == -1) {
            perror("fork failed.");
            exit(1);
        }
        if (pid == 0) { /* Child process */
            while (1) {
                printf("Child process: i=%d\n", i);
                sleep(1);
            }
            exit(0);
        } else {
            children[i] = pid;
        }
    }

    /* Parent process */
    for (i=0; i < NUMPROCS; i++) {
        sleep(3);
        if (kill(children[i], SIGTERM) == -1) {
            perror("kill failed.");
            exit(1);
        }
    }
    printf("Parent process: all children are terminated.\n");
    for (i=0; i < NUMPROCS; i++) {
        wait(&status);
    } 
}