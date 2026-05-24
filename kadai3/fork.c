#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUMPROCS 3
void main()
{
    int i, pid, status, c_pid;
    setbuf(stdout, NULL); /* set stdout to be unbufferd */
    for (i=0; i<NUMPROCS; i++) {
        if ((pid=fork())== -1) {
            perror("fork failed.");
            exit(1);
        }
        if (pid == 0) { /* Child process */
            sleep(1);
            printf("Child process i=%d\n", i);
            exit(0);
        }
    }
    /* Parent process */
    for (i=0; i<NUMPROCS; i++) {
        c_pid = wait(&status);
        printf("Parent process: child (%d)\n", c_pid);
    }
}