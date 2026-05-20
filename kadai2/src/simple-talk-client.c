#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdbool.h>

#define PORT 10130

int main(int argc, char *argv[]) {
    int client_sock, server_sock;
    struct sockaddr_in host_addr;
    struct hostent *host;
    fd_set read_fds;
    char buffer[128];

    //引数エラー
    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }

    //ソケットの作成
    if ((client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    //送り先の指定
    memset(&host_addr, 0, sizeof(host_addr)); //アドレスを0で初期化
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(PORT);
    if ((host = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "unknown host %s\n", argv[1]);
        exit(1);
    }
    memcpy(&host_addr.sin_addr, host -> h_addr_list[0], host -> h_length);

    //サーバーに接続
    if (connect(client_sock, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    do {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client_sock, &read_fds);

        struct timeval time_value;  
        time_value.tv_sec = 1;
        time_value.tv_usec = 0;

        if(select(client_sock + 1, &read_fds, NULL, NULL, &time_value) > 0) {
            memset(buffer, '\0', sizeof(buffer));
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                fgets(buffer, sizeof(buffer), stdin);
                if(buffer[strlen(buffer) - 1] == '\n') {
                    buffer[strlen(buffer) - 1] = '\0';
                }
                write(client_sock, buffer, strlen(buffer));
            }

            if (FD_ISSET(client_sock, &read_fds)) {
                int nbytes = read(client_sock, buffer, sizeof(buffer));
                if (nbytes > 0) {
                    fprintf(stdout, "server: %s\n", buffer);
                } else if (nbytes == 0) {
                    fprintf(stdout, "server disconnected\n");
                    close(client_sock);
                    break;
                } else {
                    perror("read");
                    close(client_sock);
                    break;
                }
            }
        }
    } while(true);
}