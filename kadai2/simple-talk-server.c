#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/select.h>

#define PORT 10130

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct timeval time_value;
    int max_fd = 5;

    char buffer[128];
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    //すぐ再利用可能にするためのオプション設定
    int reuse = 1;
    if(setsockopt(server_sock, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr)); //アドレスを0で初期化
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_sock, max_fd) < 0) {
        perror("listen");
        exit(1);
    }

    do {
        int client_length = sizeof(client_addr);
        if ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_length)) < 0) {
            perror("accept");
            exit(1);
        }

        struct hostent *client_host = gethostbyaddr((char *)&client_addr.sin_addr, sizeof(struct in_addr), AF_INET);
        printf("[%s]\n", client_host -> h_name);

        do {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);
            FD_SET(client_sock, &read_fds);
            time_value.tv_sec = 1;
            time_value.tv_usec = 0;
            if(select(max_fd + 1, &read_fds, NULL, NULL, &time_value) > 0) {
                if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                    fgets(buffer, sizeof(buffer), stdin);
                    if (buffer[0] == '\n') {
                        continue;
                    } else if (buffer[strlen(buffer) - 1] == '\n') {
                        buffer[strlen(buffer) - 1] = '\0';
                    }
                    write(client_sock, buffer, strlen(buffer));
                }
                
                if (FD_ISSET(client_sock, &read_fds)) {
                    int nbytes = read(client_sock, buffer, sizeof(buffer));
                    if (nbytes > 0) {
                        fprintf(stdout, "client: %s\n", buffer);
                    } else if (nbytes == 0) {
                        fprintf(stdout, "client disconnected\n");
                        close(client_sock);
                        break;
                    } else {
                        perror("read");
                        close(client_sock);
                        break;
                    }
                }
            } 
        } while (true);
    } while(true); 
}