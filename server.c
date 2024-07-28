#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080

void *handle_client(void *arg);
int connection_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
    int sockfd, new_sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // ソケットを作成する
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // ソケットにアドレスを割り当てる
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // クライアントからの接続を待つ
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        new_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (new_sockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&new_sockfd) < 0) {
            perror("ERROR creating thread");
            close(new_sockfd);
            continue;
        }
        pthread_detach(thread_id); // スレッドのリソースを自動的に解放
    }

    close(sockfd);
    return 0;
}

void *handle_client(void *arg) {
    int new_sockfd = *(int *)arg;
    char buffer[256];
    int n;

    // 接続番号を送信する
    pthread_mutex_lock(&count_mutex);
    connection_count++;
    int connection_number = connection_count;
    pthread_mutex_unlock(&count_mutex);

    snprintf(buffer, sizeof(buffer), "%d", connection_number);
    n = send(new_sockfd, buffer, strlen(buffer), 0);
    if (n < 0) {
        perror("ERROR writing to socket");
        close(new_sockfd);
        return NULL;
    }

    // データを受信する
    memset(buffer, 0, 256);
    n = recv(new_sockfd, buffer, 255, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(new_sockfd);
        return NULL;
    }

    printf("Message from client: %s\n", buffer);

    close(new_sockfd);
    return NULL;
}