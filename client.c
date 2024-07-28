#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>

#define PORT 8080
#define NUM_THREADS 5

void *client_thread(void *arg);

int main() {
    pthread_t threads[NUM_THREADS];
    int i;

    // 複数のスレッドを作成する
    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, client_thread, (void *)(intptr_t)i) != 0) {
            perror("ERROR creating thread");
            exit(1);
        }
    }

    // すべてのスレッドの終了を待つ
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

void *client_thread(void *arg) {
    int thread_id = (intptr_t)arg;
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[256];
    int n;

    // ソケットを作成する
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        pthread_exit(NULL);
    }

    // サーバーのアドレスを設定する
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // サーバーのIPアドレス
    serv_addr.sin_port = htons(PORT);

    // サーバーに接続する
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);
        pthread_exit(NULL);
    }

    // 接続番号を受信する
    memset(buffer, 0, 256);
    n = recv(sockfd, buffer, 255, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("Thread %d received connection number: %s\n", thread_id, buffer);

    // メッセージを送信する
    snprintf(buffer, sizeof(buffer), "Hello from client %d", thread_id);
    n = send(sockfd, buffer, strlen(buffer), 0);
    if (n < 0) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    close(sockfd);
    pthread_exit(NULL);
}