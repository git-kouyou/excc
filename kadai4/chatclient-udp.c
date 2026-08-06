#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define PORT 10140

#define MAX_BUFFER_SIZE 128

#define ACK_MSG "REQUEST ACCEPTED\n"
#define USERNAME_ACK_MSG "USERNAME REGISTERED\n"

void handle_error(int sock);

static void send_datagram(int sock, const char *message, size_t length) {
    if (send(sock, message, length, 0) < 0) {
        handle_error(sock);
    }
}

static int receive_datagram(int sock, char *buffer, size_t buffer_size) {
    int bytes_received = recv(sock, buffer, buffer_size - 1, 0);
    if (bytes_received < 0) {
        handle_error(sock);
    }
    buffer[bytes_received] = '\0';
    return bytes_received;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <username>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_BUFFER_SIZE];
    const char *server_ip = argv[1];
    const char *username = argv[2];

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP: %s\n", server_ip);
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", server_ip, PORT);

    send_datagram(sock, "HELLO\n", strlen("HELLO\n"));
    if (receive_datagram(sock, buffer, sizeof(buffer)) <= 0 || strcmp(buffer, ACK_MSG) != 0) {
        fprintf(stderr, "Connection rejected\n");
        handle_error(sock);
    }

    strncpy(buffer, username, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    size_t username_length = strlen(buffer);
    buffer[username_length] = '\n';
    send_datagram(sock, buffer, username_length + 1);

    if (receive_datagram(sock, buffer, sizeof(buffer)) <= 0 || strcmp(buffer, USERNAME_ACK_MSG) != 0) {
        fprintf(stderr, "Username rejected\n");
        handle_error(sock);
    }

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);

        int max_fd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                send_datagram(sock, buffer, strlen(buffer));
            }
        }

        if (FD_ISSET(sock, &read_fds)) {
            int n = receive_datagram(sock, buffer, sizeof(buffer));
            if (n <= 0) {
                printf("Server disconnected\n");
                break;
            }
            printf("%s", buffer);
        }
    }

    close(sock);
    return 0;
}

void handle_error(int sock) {
    close(sock);
    exit(EXIT_FAILURE);
}