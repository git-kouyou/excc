#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 10140
#define MAXCLIENT 5

#define MAX_USERNAME_LENGTH 16
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
    struct sockaddr_in addr;
    char name[MAX_USERNAME_LENGTH];
    ClientStatus status;
} ClientInfo;

typedef struct _ClientList {
    ClientInfo clients[MAXCLIENT];
    int client_count;
    int max_client;
} ClientList;

static bool same_addr(struct sockaddr_in *a, struct sockaddr_in *b);
static int find_client_by_addr(ClientList *client_list, struct sockaddr_in *addr);
static int find_free_slot(ClientList *client_list);

void receive_username(ClientList* client_list, ClientInfo* client, int server_sock, const char *message, size_t message_length);
void new_client(ClientList* client_list, int server_sock, struct sockaddr_in *addr, const char *message, size_t message_length);
void register_client_name(ClientList* client_list, ClientInfo* client, int server_sock, const char *name);
void client_disconnected(ClientList* client_list, ClientInfo* client);
void broadcast_message(ClientList* client_list, ClientInfo* sender, int server_sock, const char *message, size_t message_length);
void write_all_clients(ClientList* client_list, ClientInfo* sender, int server_sock, const char *message, size_t message_length);
void write_other_clients(ClientInfo clients[], int max_clients, int server_sock, const char *message, size_t message_length, ClientInfo *sender);
void handle_massage(ClientList* client_list, ClientInfo* sender, int server_sock, const char *message, size_t message_length);
void all_username(ClientList* client_list, ClientInfo* sender, int server_sock);

int main() {
    int server_sock;
    char buffer[MAX_BUFFER_SIZE];

    ClientList client_list;
    client_list.client_count = 0;
    client_list.max_client = MAXCLIENT;
    for (int i = 0; i < MAXCLIENT; i++) {
        memset(&client_list.clients[i].addr, 0, sizeof(client_list.clients[i].addr));
        strcpy(client_list.clients[i].name, "");
        client_list.clients[i].status = NONE;
    }

    struct sockaddr_in server_addr;
    struct timeval time_value;

    if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    int reuse = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
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

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);
        time_value.tv_sec = 1;
        time_value.tv_usec = 0;

        if (select(server_sock + 1, &read_fds, NULL, NULL, &time_value) < 0) {
            perror("select");
            exit(1);
        }

        if (!FD_ISSET(server_sock, &read_fds)) {
            continue;
        }

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int bytes_received = recvfrom(server_sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client_addr, &addr_len);

        if (bytes_received <= 0) {
            continue;
        }

        buffer[bytes_received] = '\0';

        int index = find_client_by_addr(&client_list, &client_addr);
        if (index < 0) {
            new_client(&client_list, server_sock, &client_addr, buffer, (size_t)bytes_received);
            continue;
        }

        ClientInfo *client = &client_list.clients[index];
        if (client->status == CONNECTING) {
            receive_username(&client_list, client, server_sock, buffer, (size_t)bytes_received);
        } else if (client->status == CONNECTED) {
            handle_massage(&client_list, client, server_sock, buffer, (size_t)bytes_received);
        }
    }
}

static bool same_addr(struct sockaddr_in *a, struct sockaddr_in *b) {
    return a->sin_family == b->sin_family &&
           a->sin_port == b->sin_port &&
           a->sin_addr.s_addr == b->sin_addr.s_addr;
}

static int find_client_by_addr(ClientList *client_list, struct sockaddr_in *addr) {
    for (int i = 0; i < client_list->max_client; i++) {
        if (client_list->clients[i].status != NONE && same_addr(&client_list->clients[i].addr, addr)) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(ClientList *client_list) {
    for (int i = 0; i < client_list->max_client; i++) {
        if (client_list->clients[i].status == NONE) {
            return i;
        }
    }
    return -1;
}

void new_client(ClientList* client_list, int server_sock, struct sockaddr_in *addr, const char *message, size_t message_length) {
    (void)message;
    (void)message_length;
    int free_index = find_free_slot(client_list);

    if (free_index < 0 || client_list->client_count >= client_list->max_client) {
        sendto(server_sock, REJECT_MSG, strlen(REJECT_MSG), 0, (struct sockaddr *)addr, sizeof(*addr));
        printf("Max clients reached. Rejecting new client.\n");
        return;
    }

    client_list->clients[free_index].addr = *addr;
    client_list->clients[free_index].status = CONNECTING;
    strcpy(client_list->clients[free_index].name, "");
    client_list->client_count++;

    sendto(server_sock, ACK_MSG, strlen(ACK_MSG), 0, (struct sockaddr *)addr, sizeof(*addr));
}

void receive_username(ClientList* client_list, ClientInfo* client, int server_sock, const char *message, size_t message_length) {
    (void)message_length;
    char buffer[MAX_USERNAME_LENGTH];
    strncpy(buffer, message, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *newline = strchr(buffer, '\n');
    if (newline != NULL) {
        *newline = '\0';
    }

    register_client_name(client_list, client, server_sock, buffer);
}

void register_client_name(ClientList* client_list, ClientInfo* client, int server_sock, const char *name) {
    for (int i = 0; i < client_list->max_client; i++) {
        if (client_list->clients[i].status == CONNECTED && strcmp(client_list->clients[i].name, name) == 0) {
            sendto(server_sock, USERNAME_REJECT_MSG, strlen(USERNAME_REJECT_MSG), 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
            client->status = NONE;
            client->name[0] = '\0';
            client_list->client_count--;
            return;
        }
    }

    strncpy(client->name, name, MAX_USERNAME_LENGTH - 1);
    client->name[MAX_USERNAME_LENGTH - 1] = '\0';
    client->status = CONNECTED;

    printf("Registered username: %s\n", client->name);

    sendto(server_sock, USERNAME_ACK_MSG, strlen(USERNAME_ACK_MSG), 0, (struct sockaddr *)&client->addr, sizeof(client->addr));

    char join_msg_template[] = "%s joined\n";
    char join_msg[MAX_USERNAME_LENGTH + sizeof(join_msg_template)];
    snprintf(join_msg, sizeof(join_msg), join_msg_template, client->name);
    write_all_clients(client_list, client, server_sock, join_msg, strlen(join_msg));
}

void client_disconnected(ClientList* client_list, ClientInfo* client) {
    (void)client_list;
    client->status = NONE;
    client->name[0] = '\0';
    memset(&client->addr, 0, sizeof(client->addr));
}

void handle_massage(ClientList* client_list, ClientInfo* sender, int server_sock, const char *message, size_t message_length) {
    if (message_length == 0) {
        return;
    }

    printf("%s: %s", sender->name, message);

    if (strcmp(message, "/list\n") == 0 || strcmp(message, "/list") == 0) {
        all_username(client_list, sender, server_sock);
        return;
    }

    broadcast_message(client_list, sender, server_sock, message, message_length);
}

void all_username(ClientList* client_list, ClientInfo* sender, int server_sock) {
    char username[MAX_BUFFER_SIZE];

    printf("All usernames:\n");
    for (int i = 0; i < client_list->max_client; i++) {
        if (client_list->clients[i].status == CONNECTED) {
            printf("%s\n", client_list->clients[i].name);
            snprintf(username, sizeof(username), "%s\n", client_list->clients[i].name);
            sendto(server_sock, username, strlen(username), 0, (struct sockaddr *)&sender->addr, sizeof(sender->addr));
        }
    }
}

void broadcast_message(ClientList* client_list, ClientInfo* sender, int server_sock, const char *message, size_t message_length) {
    char formatted_message[MAX_USERNAME_LENGTH + MAX_BUFFER_SIZE + 8];
    snprintf(formatted_message, sizeof(formatted_message), "%s: %s", sender->name, message);
    write_all_clients(client_list, sender, server_sock, formatted_message, strlen(formatted_message));
}

void write_all_clients(ClientList* client_list, ClientInfo* sender, int server_sock, const char *message, size_t message_length) {
    for (int i = 0; i < client_list->max_client; i++) {
        if (client_list->clients[i].status == CONNECTED) {
            sendto(server_sock, message, message_length, 0, (struct sockaddr *)&client_list->clients[i].addr, sizeof(client_list->clients[i].addr));
        }
    }
}

void write_other_clients(ClientInfo clients[], int max_clients, int server_sock, const char *message, size_t message_length, ClientInfo *sender) {
    for (int i = 0; i < max_clients; i++) {
        if (clients[i].status == CONNECTED && !same_addr(&clients[i].addr, &sender->addr)) {
            sendto(server_sock, message, message_length, 0, (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr));
        }
    }
}