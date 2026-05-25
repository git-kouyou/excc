#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 256
#define TIMEOUT 5

void myalarm(unsigned int sec) {
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    kill(0, SIGTERM);

    int pid = fork();
    if (pid == -1) {
        perror("fork failed.");
        exit(1);
    }

    if (pid == 0) { /* Child process */
        signal(SIGTERM, SIG_DFL);
        sleep(sec);
        kill(getppid(), SIGALRM);
        exit(0);
    }
}

void timeout() {
    printf("This program is timeout.\n");
    exit(0);
}

int main() {
    char buf[BUFSIZE];
    if(signal(SIGALRM, timeout) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }

    myalarm(TIMEOUT);
    while (fgets(buf, BUFSIZE, stdin) != NULL) {
        printf("echo: %s", buf);
        myalarm(TIMEOUT);
    }
}