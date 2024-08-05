#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define NUM_CLIENTS 10

ssize_t read_all(int sock, void *buffer, size_t length) {
    size_t total_read = 0;
    while (total_read < length) {
        ssize_t bytes_read = read(sock, (char *)buffer + total_read, length - total_read);
        if (bytes_read <= 0) {
            return bytes_read;
        }
        total_read += bytes_read;
    }
    return total_read;
}

ssize_t write_all(int sock, const void *buffer, size_t length) {
    size_t total_written = 0;
    while (total_written < length) {
        ssize_t bytes_written = write(sock, (const char *)buffer + total_written, length - total_written);
        if (bytes_written <= 0) {
            return bytes_written;
        }
        total_written += bytes_written;
    }
    return total_written;
}

void *client_thread(void *arg) {
    struct sockaddr_in serv_addr;
    int sock = 0;
    time_t start_time = time(NULL);

    while (difftime(time(NULL), start_time) < 60) {
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\nSocket creation error\n");
            return NULL;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported\n");
            close(sock);
            continue;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("\nConnection Failed\n");
            close(sock);
            exit(1);
        }

        int connection_number;
        if (read_all(sock, &connection_number, sizeof(connection_number)) <= 0) {
            printf("\nFailed to read connection number\n");
            close(sock);
            continue;
        }
        printf("Received connection number: %d\n", connection_number);

        int *random_numbers = malloc(connection_number * sizeof(int));
        for (int i = 0; i < connection_number; i++) {
            random_numbers[i] = rand() % 10000; // 乱数の上限を10000に設定
        }

        printf("Sending random numbers: ");
        for (int i = 0; i < connection_number; i++) {
            printf("%d ", random_numbers[i]);
        }
        printf("\n");

        if (write_all(sock, random_numbers, connection_number * sizeof(int)) <= 0) {
            printf("\nFailed to send random numbers\n");
            free(random_numbers);
            close(sock);
            continue;
        }

        if (read_all(sock, random_numbers, connection_number * sizeof(int)) <= 0) {
            printf("\nFailed to read sorted numbers\n");
            free(random_numbers);
            close(sock);
            continue;
        }
        printf("Received sorted numbers: ");
        for (int i = 0; i < connection_number; i++) {
            printf("%d ", random_numbers[i]);
        }
        printf("\n");

        free(random_numbers);
        close(sock);

        // 少し待機してから次のリクエストを送信
        usleep(100000); // 0.1秒待機
    }

    return NULL;
}

int main() {
    pthread_t clients[NUM_CLIENTS];

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_create(&clients[i], NULL, client_thread, NULL);
    }

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(clients[i], NULL);
    }

    return 0;
}
