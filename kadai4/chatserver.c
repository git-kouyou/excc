#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 10140
#define MAXCLIENT 5

#define MAX_USERNAME_LENGTH 16 // ヌル文字を含むユーザーネーム最大長
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
    char name[MAX_USERNAME_LENGTH];
    ClientStatus status;
} ClientInfo;

typedef struct _ClientList {
    ClientInfo clients[MAXCLIENT];
    int client_count;
    int max_client;
} ClientList;

int flush_msg(int socket);

void receive_username();
void new_client();
void register_client_name();
void client_disconnected();
void broadcast_message();
void write_all_clients();
void write_other_clients();

int main() {
    int server_sock;
    char buffer[MAX_BUFFER_SIZE];

    ClientList client_list;
    client_list.client_count = 0;
    client_list.max_client = MAXCLIENT;
    for(int i = 0; i < MAXCLIENT; i++) {
        client_list.clients[i].socket = 0;
        strcpy(client_list.clients[i].name, "");
        client_list.clients[i].status = NONE;
    }

    struct sockaddr_in server_addr;
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

    if (listen(server_sock, MAXCLIENT) < 0) {
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
        for (int i = 0; i < MAXCLIENT; i++) {
            int sd = client_list.clients[i].socket;
            
            if (client_list.clients[i].status != NONE) {
                FD_SET(sd, &read_fds);
            }
            
            // max_fdの更新
            if (sd > max_fd) {
                max_fd = sd;
            }
        }

        ClientInfo* clients = client_list.clients;
        // select関数で監視
        if(select(max_fd + 1, &read_fds, NULL, NULL, &time_value) < 0) {
            perror("select");
            exit(1);
        } else {
            if(FD_ISSET(server_sock, &read_fds)) {
                // 新しいクライアントからの接続要求
                new_client(&client_list, server_sock);
            } else {
                // 既存のクライアントからのデータ受信
                for (int i = 0; i < MAXCLIENT; i++) {
                    if(client_list.clients[i].status == NONE) {
                        continue; // クライアントが接続されていない場合はスキップ
                    }
                    int sd = client_list.clients[i].socket;

                    if (FD_ISSET(sd, &read_fds)) {
                        // クライアントが接続中の場合、名前を受信
                        if(client_list.clients[i].status == CONNECTING) {
                            receive_username(&client_list, &client_list.clients[i]);
                        } else if(client_list.clients[i].status == CONNECTED) {
                            broadcast_message(&client_list, &client_list.clients[i]);
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

void new_client(ClientList* client_list, int server_sock) {
    // 新しいクライアントからの接続要求があった場合
    ClientInfo *clients = client_list -> clients;
    int* client_count = &(client_list->client_count);
    int max_client = client_list -> max_client;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int new_client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);

    if (new_client_sock < 0) {
        perror("accept");
        exit(1);
    }
    //接続拒否
    if(*client_count >= max_client) {
        printf("Current clients: %d, Max clients: %d\n", *client_count, max_client);
        printf("Max clients reached. Rejecting new connection.\n");
        write(new_client_sock, REJECT_MSG, strlen(REJECT_MSG));
        close(new_client_sock);
    // 接続許可
    } else {
        for (int i = 0; i < max_client; i++) {
            if (clients[i].status == NONE) {
                clients[i].socket = new_client_sock;
                clients[i].status = CONNECTING;
                write(new_client_sock, ACK_MSG, strlen(ACK_MSG));
                printf("New client connected: socket fd is %d\n", new_client_sock);
                (*client_count)++;
                return;
            }
        }
    }  
}

void receive_username(ClientList* client_list, ClientInfo* client) {
    ClientInfo* clients = client_list->clients;

    char buffer[MAX_USERNAME_LENGTH];
    int bytes_received = read(client->socket, buffer, MAX_USERNAME_LENGTH);
    printf("Received username: %dbyte\n", bytes_received);

    // 切断時
    if (bytes_received <= 0) {
        client_disconnected(client_list, client);
    } else {
        if(strchr(buffer, '\n') == NULL) {
            buffer[MAX_USERNAME_LENGTH - 1] = '\0'; // null終端
        } else {
            *strchr(buffer, '\n') = '\0'; // 改行文字を削除
        }
        register_client_name(client_list, client, buffer);
    }
}

void register_client_name(ClientList* client_list, ClientInfo* client, const char *name) {
    ClientInfo* clients = client_list->clients;
    int* client_count = &(client_list->client_count);
    int max_clients = client_list->max_client;

    for(int i = 0; i < max_clients; i++) {
        if (clients[i].status == CONNECTED && strcmp(clients[i].name, name) == 0) {
            // 名前が重複している場合
            write(client->socket, USERNAME_REJECT_MSG, strlen(USERNAME_REJECT_MSG));
            close(client->socket);
            client->socket = 0;
            strcpy(client->name, "");
            client->status = NONE;
            return;
        }
    }
    client->name[MAX_USERNAME_LENGTH - 1] = '\0'; // null終端
    strncpy(client -> name, name, MAX_USERNAME_LENGTH - 1);

    char join_msg_template[] = "%s joined\n";
    char join_msg[MAX_USERNAME_LENGTH + strlen(join_msg_template)];
    snprintf(join_msg, sizeof(join_msg), join_msg_template, client -> name);

    client->status = CONNECTED;
    write(client->socket, USERNAME_ACK_MSG, strlen(USERNAME_ACK_MSG));
    write_all_clients(clients, max_clients, join_msg);
}

void client_disconnected(ClientList* client_list, ClientInfo* client) {
    ClientInfo* clients = client_list->clients;
    int max_clients = client_list->max_client;
    int* client_count = &(client_list->client_count);

    char disconnect_message_template[] = "%s is disconnected\n";

    if(client->status == CONNECTING) {
        close(client->socket);
        client->socket = 0;
        client->status = NONE;
        *client_count--;
        printf("Unregistered client is disconnected\n");
        return;
    }
    
    char buffer[MAX_USERNAME_LENGTH + strlen(disconnect_message_template)];
    snprintf(buffer, sizeof(buffer), disconnect_message_template, client->name);
    close(client->socket);
    client->socket = 0;
    client->status = NONE;
    *client_count--;
    write_all_clients(clients, max_clients, buffer);
}

void broadcast_message(ClientList* client_list, ClientInfo* sender) {
    ClientInfo* clients = client_list->clients;
    int* client_count = &(client_list->client_count);

    char message_format[] = "%s: %s";
    char message_buffer[MAX_BUFFER_SIZE];
    char formatted_message[MAX_USERNAME_LENGTH + MAX_BUFFER_SIZE + strlen(message_format)];
    int bytes_received = read(sender->socket, message_buffer, sizeof(message_buffer) - sizeof(char));
    message_buffer[bytes_received] = '\0'; // null終端
    printf("Received message: %s\n", message_buffer);

    if (bytes_received <= 0) {
        client_disconnected(client_list, sender);
        return;
    } 
    printf("%s: %s", sender->name, message_buffer);
    snprintf(formatted_message, sizeof(formatted_message), message_format, sender->name, message_buffer);
    printf("Broadcasting message: %s", formatted_message);
    write_all_clients(client_list, sender, formatted_message);
}

void write_all_clients(ClientList* client_list, ClientInfo* sender, const char *message) {
    ClientInfo* clients = client_list->clients;
    int max_clients = client_list->max_client;

    for (int i = 0; i < max_clients; i++) {
        if (clients[i].status == CONNECTED) {
            write(clients[i].socket, message, strlen(message) * sizeof(char));
        }
    }
}

void write_other_clients(ClientInfo clients[], int max_clients, const char *message, size_t message_length, ClientInfo *sender) {
    for (int i = 0; i < max_clients; i++) {
        if (clients[i].status == CONNECTED && &clients[i] != sender) {
            write(clients[i].socket, message, message_length * sizeof(char));
        }
    }
}