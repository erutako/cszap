#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8080
#define MAX_CLIENTS 1000

int server_fd;
int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t stop_server = 0;

void handle_sigint(int sig) {
    stop_server = 1;
    close(server_fd);
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

    pthread_mutex_lock(&client_count_mutex);
    int connection_number = ++client_count;
    pthread_mutex_unlock(&client_count_mutex);

    printf("Sending connection number: %d\n", connection_number);
    write(client_socket, &connection_number, sizeof(connection_number));

    int *random_numbers = malloc(connection_number * sizeof(int));
    read(client_socket, random_numbers, connection_number * sizeof(int));

    printf("Received random numbers: ");
    for (int i = 0; i < connection_number; i++) {
        printf("%d ", random_numbers[i]);
    }
    printf("\n");

    sort(random_numbers, connection_number);

    printf("Sending sorted numbers: ");
    for (int i = 0; i < connection_number; i++) {
        printf("%d ", random_numbers[i]);
    }
    printf("\n");

    write(client_socket, random_numbers, connection_number * sizeof(int));

    free(random_numbers);
    close(client_socket);

    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

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

    // Wait for all clients to finish
    while (1) {
        pthread_mutex_lock(&client_count_mutex);
        if (client_count == 0) {
            pthread_mutex_unlock(&client_count_mutex);
            break;
        }
        pthread_mutex_unlock(&client_count_mutex);
        sleep(1);
    }

    close(server_fd);
    return 0;
}
