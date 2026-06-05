#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#define PORT 10120

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in host;
    struct hostent *hp;

    //引数エラー
    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }

    //ソケットの生成
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    //送り先の指定
    bzero(&host, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(PORT);
    if ((hp = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "unknown host %s\n", argv[1]);
        exit(1);
    }
    bcopy(hp -> h_addr_list[0], &host.sin_addr, hp -> h_length);

    //サーバーに接続
    if (connect(sock, (struct sockaddr *)&host, sizeof(host)) < 0) {
        perror("connect");
        exit(1);
    }

    while(true) {
        char buf[1024]; 
        printf("Enter message: ");
        //EOFで終了
        if(fgets(buf, sizeof(buf), stdin) == NULL) {
            //見た目が悪いので改行
            printf("\n");
            break; // EOF or error
        }
        write(sock, buf, strlen(buf));
        read(sock, buf, sizeof(buf));
        printf("Received: %s\n", buf);
    }
    
    close(sock);
}