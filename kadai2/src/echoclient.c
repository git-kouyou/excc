#include <sys/types.h> /* socket() を使うために必要*/
#include <sys/socket.h> /* 同上*/
#include <netinet/in.h> /* INET ドメインのソケットを使うために必要*/
#include <netdb.h> /* gethostbyname() を使うために必要*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#define PORT 10000

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in host;
    struct hostent *hp;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }

    /* ソケットの生成*/
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    /* host(ソケットの接続先) の情報設定*/
    bzero(&host, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(PORT);
    if ((hp = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "unknown host %s\n", argv[1]);
        exit(1);
    }

    bcopy(hp -> h_addr_list[0], &host.sin_addr, hp -> h_length);
    if (connect(sock, (struct sockaddr *)&host, sizeof(host)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    while(true) {
        char send_msg[1024];
        char recv_msg[1024];
        printf("Enter message: ");
        if (fgets(send_msg, sizeof(send_msg), stdin) == NULL) break;
        ssize_t w = write(sock, send_msg, strlen(send_msg));
        if (w < 0) { perror("write"); break; }
        ssize_t n = read(sock, recv_msg, sizeof(recv_msg) - 1);
        if (n < 0) { perror("read"); break; }
        if (n == 0) { fprintf(stderr, "server closed connection\n"); break; }
        recv_msg[n] = '\0';
        printf("Received: %s\n", recv_msg);
    }
    
    close(sock);
}