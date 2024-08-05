#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8080
#define MAX_CLIENTS 10

int server_fd;
int connection_number = 0;
pthread_mutex_t connection_number_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t stop_server = 0;

void handle_sigint(int sig) {
    stop_server = 1;
    close(server_fd);
}

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

void sort(int *array, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (array[j] > array[j + 1]) {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&connection_number_mutex);
    int current_connection_number = ++connection_number;
    pthread_mutex_unlock(&connection_number_mutex);

    printf("Sending connection number: %d\n", current_connection_number);
    if (write_all(client_socket, &current_connection_number, sizeof(current_connection_number)) <= 0) {
        printf("Failed to send connection number\n");
        close(client_socket);
        return NULL;
    }

    int *random_numbers = malloc(current_connection_number * sizeof(int));
    if (read_all(client_socket, random_numbers, current_connection_number * sizeof(int)) <= 0) {
        printf("Failed to read random numbers\n");
        free(random_numbers);
        close(client_socket);
        return NULL;
    }

    printf("Received random numbers: ");
    for (int i = 0; i < current_connection_number; i++) {
        printf("%d ", random_numbers[i]);
    }
    printf("\n");

    sort(random_numbers, current_connection_number);

    printf("Sending sorted numbers: ");
    for (int i = 0; i < current_connection_number; i++) {
        printf("%d ", random_numbers[i]);
    }
    printf("\n");

    if (write_all(client_socket, random_numbers, current_connection_number * sizeof(int)) <= 0) {
        printf("Failed to send sorted numbers\n");
        free(random_numbers);
        close(client_socket);
        return NULL;
    }

    free(random_numbers);
    close(client_socket);

    return NULL;
}

int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    signal(SIGINT, handle_sigint);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while (!stop_server) {
        int *new_socket = malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            if (stop_server) break;
            perror("accept failed");
            free(new_socket);
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, new_socket);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
