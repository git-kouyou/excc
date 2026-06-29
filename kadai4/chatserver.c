#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 10140
#define MAXCLIENTS 5

#define MAX_USERNAME_LENGTH 15
#define MAX_BUFFER_SIZE 128

#define ACK_MSG "REQUEST ACCEPTED\n"
#define REJECT_MSG "REQUEST REJECTED\n"

#define USERNAME_ACK_MSG "USERNAME REGISTERED\n"
#define USERNAME_REJECT_MSG "USERNAME REJECTED\n"

typedef enum CLIENT_STATUS {
    NONE,
    CONNECTING,
    CONNECTED
} ClientStatus;

typedef struct {
    int socket;
    char name[MAX_USERNAME_LENGTH + 1];
    ClientStatus status;
} ClientInfo;

int flush_msg(int socket);
void register_client_name();
void client_disconnected();
void write_all_clients();
void write_other_clients();

int main() {
    int server_sock;
    char buffer[MAX_BUFFER_SIZE];

    ClientInfo clients[MAXCLIENTS];
    for(int i = 0; i < MAXCLIENTS; i++) {
        clients[i].socket = 0;
        strcpy(clients[i].name, "");
        clients[i].status = NONE;
    }

    int client_count = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct timeval time_value;

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    //すぐ再利用可能にするためのオプション設定
    int reuse = 1;
    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_sock, MAXCLIENTS) < 0) {
        perror("listen");
        exit(1);
    }

    while(true) {
        int max_fd = server_sock; // 最大のソケット番号を保持する変数
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);
        time_value.tv_sec = 1;
        time_value.tv_usec = 0;

        // クライアントのソケットを監視対象に追加
        for (int i = 0; i < MAXCLIENTS; i++) {
            int sd = clients[i].socket;
            
            if (clients[i].status != NONE) {
                FD_SET(sd, &read_fds);
            }
            
            // max_fdの更新
            if (sd > max_fd) {
                max_fd = sd;
            }
        }

        // select関数で監視
        if(select(max_fd + 1, &read_fds, NULL, NULL, &time_value) < 0) {
            perror("select");
            exit(1);
        } else {
            if(FD_ISSET(server_sock, &read_fds)) {
                // 新しいクライアントからの接続要求があった場合
                socklen_t addr_len = sizeof(client_addr);
                int new_client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
                if (new_client_sock < 0) {
                    perror("accept");
                    exit(1);
                }

                // 接続拒否
                if(client_count >= MAXCLIENTS) {
                    printf("Max clients reached. Rejecting new connection.\n");
                    write(new_client_sock, REJECT_MSG, strlen(REJECT_MSG));
                    close(new_client_sock);
                // 接続許可
                } else {
                    for (int i = 0; i < MAXCLIENTS; i++) {
                        if (clients[i].status == NONE) {
                            clients[i].socket = new_client_sock;
                            clients[i].status = CONNECTING;
                            write(new_client_sock, ACK_MSG, strlen(ACK_MSG));
                            printf("New client connected: socket fd is %d\n", new_client_sock);
                            client_count++;
                            break;
                        }
                    }
                }  
            } else {
                // 既存のクライアントからのデータ受信
                for (int i = 0; i < MAXCLIENTS; i++) {
                    if(clients[i].status == NONE) {
                        continue; // クライアントが接続されていない場合はスキップ
                    }
                    int sd = clients[i].socket;

                    if (FD_ISSET(sd, &read_fds)) {
                        // クライアントが接続中の場合、名前を受信
                        if(clients[i].status == CONNECTING) {
                            int bytes_received = read(sd, buffer, MAX_USERNAME_LENGTH);
                            // クライアントが切断した場合
                            if (bytes_received <= 0) {
                                client_disconnected(clients, MAXCLIENTS, i, &client_count);
                            } else {
                                buffer[bytes_received] = '\0';
                                register_client_name(clients, MAXCLIENTS, i, buffer);
                            }
                            
                        } else if(clients[i].status == CONNECTED) {
                            int bytes_received = read(sd, buffer, sizeof(buffer));
                            if (bytes_received <= 0) {
                                client_disconnected(clients, MAXCLIENTS, i, &client_count);
                            } else {
                                snprintf(buffer, sizeof(buffer), "%s: %s", clients[i].name, buffer);

                                write_all_clients(clients, MAXCLIENTS, buffer, bytes_received);
                                printf("%s: %s\n", clients[i].name, buffer);
                            }
                        }
                    }
                }
            }
        }
    }
}

int flush_msg(int socket) {
    int bytes_sum = 0;
    char buffer[MAX_BUFFER_SIZE];
    while (true) {
        int bytes_received = read(socket, buffer, sizeof(buffer));
        if (bytes_received <= 0) {
            break; 
        }
        
        if(bytes_received < sizeof(buffer)) {
            bytes_sum += bytes_received;
            break; 
        } else {
            bytes_sum += bytes_received;
        }
    }
    return bytes_sum;
}

void register_client_name(ClientInfo* clients, int max_clients, int client_index, const char *name) {
    ClientInfo* client = &clients[client_index];
    flush_msg(client->socket); // 受信バッファをクリア
    strncpy(client->name, name, MAX_USERNAME_LENGTH);
    client->name[MAX_USERNAME_LENGTH] = '\0'; // null終端
    for(int i = 0; i < max_clients; i++) {
        if (clients[i].status == CONNECTED && strcmp(clients[i].name, name) == 0) {
            // 名前が重複している場合
            write(client->socket, USERNAME_REJECT_MSG, strlen(USERNAME_REJECT_MSG));
            close(client->socket);
            client->socket = 0;
            client->status = NONE;
            return;
        }
    }
    client->status = CONNECTED;
    write(client->socket, USERNAME_ACK_MSG, strlen(USERNAME_ACK_MSG));
}

void client_disconnected(ClientInfo* clients, int max_clients, int client_index, int *client_count) {
    ClientInfo* client = &clients[client_index];
    printf("%s is disconnected\n", client->name);
    char buffer[MAX_USERNAME_LENGTH + 20]; // " is disconnected" の長さを考慮
    sprintf(buffer, "%s is disconnected", client->name);
    close(client->socket);
    client->socket = 0;
    client->status = NONE;
    (*client_count)--;
    write_all_clients(clients, max_clients, buffer, sizeof(buffer));
}

void write_all_clients(ClientInfo clients[], int max_clients, const char *message, size_t message_length) {
    for (int i = 0; i < max_clients; i++) {
        if (clients[i].status == CONNECTED) {
            write(clients[i].socket, message, message_length);
        }
    }
}

void write_other_clients(ClientInfo clients[], int max_clients, const char *message, size_t message_length, ClientInfo *sender) {
    for (int i = 0; i < max_clients; i++) {
        if (clients[i].status == CONNECTED && &clients[i] != sender) {
            write(clients[i].socket, message, message_length);
        }
    }
}