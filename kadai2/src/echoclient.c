#include <sys/types.h> /* socket() を使うために必要*/
#include <sys/socket.h> /* 同上*/
#include <netinet/in.h> /* INET ドメインのソケットを使うために必要*/
#include <netdb.h> /* gethostbyname() を使うために必要*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define PORT 10000

int main() {
    int sock;
    struct sockaddr_in host;
    struct hostent *hp;

    /* ソケットの生成*/
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    connect(sock, (struct sockaddr *)&host, sizeof(host));

    while(true) {
        char buf[1024];
        printf("Enter message: ");
        fgets(buf, sizeof(buf), stdin);
        sendto(sock, buf, strlen(buf), 0, (struct sockaddr*)&host, sizeof(host));
    }
}