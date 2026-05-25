#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#define BUFSIZE 256
void sigmsg() {
    printf("\n!! CTRL-C is pressed !!\n\n");
    if(signal(SIGINT,sigmsg) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }
}

int main() {
    char buf[BUFSIZE];
    if(signal(SIGINT, sigmsg) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }
    while (fgets(buf, BUFSIZE, stdin) != NULL) {
        printf("echo: %s",buf);
    }
}