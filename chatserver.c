#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>  // Added for timestamp

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048

// Structure to hold client information
typedef struct {
    int socket;
    struct sockaddr_in address;
    char username[50];
} client_t;

client_t *clients[MAX_CLIENTS];  // Array of pointers to client structs
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_socket;
FILE *chat_history; // File pointer for chat history

// Function to add timestamp to message
void add_timestamp(char *buffer) {
    time_t now;
    struct tm *time_info;
    char timestamp[30];

    time(&now);
    time_info = localtime(&now);
    strftime(timestamp, 30, "[%Y-%m-%d %H:%M:%S] ", time_info);

    char temp[BUFFER_SIZE];
    strcpy(temp, buffer);
    strcpy(buffer, timestamp);
    strcat(buffer, temp);
}

// Add client to the array
void add_client(client_t *client) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Remove client from the array
void remove_client(int socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket == socket) {
            free(clients[i]);
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Broadcast message to all clients except sender
void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket != sender_socket) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("ERROR: Failed to send message");
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Save message to chat history file
void save_to_history(char *message) {
    pthread_mutex_lock(&file_mutex);
    fprintf(chat_history, "%s", message);
    fflush(chat_history);
    pthread_mutex_unlock(&file_mutex);
}

// Handle communication with a client
void *handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    int leave_flag = 0;

    client_t *client = (client_t *)arg;

    // Receive username
    if (recv(client->socket, client->username, 50, 0) <= 0) {
        printf("Did not get the username.\n");
        leave_flag = 1;
    } else {
        sprintf(message, "%s has joined the chat.\n", client->username);
        printf("%s", message);
        add_timestamp(message);
        save_to_history(message);
        broadcast_message(message, client->socket);
    }

    // Loop to receive messages from the client
    while (!leave_flag) {
        int receive = recv(client->socket, buffer, BUFFER_SIZE, 0);
        memset(message, 0, BUFFER_SIZE);

        if (receive > 0) {
            if (strlen(buffer) > 0) {
                snprintf(message, BUFFER_SIZE, "%.*s:%.*s", 49, client->username, BUFFER_SIZE - 52, buffer);
                printf("%s", message);
                add_timestamp(message);
                save_to_history(message);
                broadcast_message(message, client->socket);
            }
        } else if (receive == 0 || strcmp(buffer, "exit\n") == 0) {
            sprintf(message, "%s has left the chat.\n", client->username);
            printf("%s", message);
            add_timestamp(message);
            save_to_history(message);
            broadcast_message(message, client->socket);
            leave_flag = 1;
        } else {
            perror("ERROR: Failed to receive");
            leave_flag = 1;
        }

        memset(buffer, 0, BUFFER_SIZE);
    }

    // Clean up
    close(client->socket);
    remove_client(client->socket);
    pthread_detach(pthread_self());
    return NULL;
}

// Handle SIGINT (Ctrl+C) for graceful shutdown
void handle_signal(int sign) {
    if (sign == SIGINT) {
        printf("\nShutting down server...\n");

        // Close client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i]) {
                close(clients[i]->socket);
                free(clients[i]);
                clients[i] = NULL;
            }
        }

        // Close server socket and chat history file
        if (server_socket) close(server_socket);
        if (chat_history) fclose(chat_history);

        printf("Server shutdown complete.\n");
        exit(0);
    }
}

// Count current connected clients
int client_count() {
    int count = 0;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) count++;
    }
    pthread_mutex_unlock(&clients_mutex);
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int option = 1;
    struct sockaddr_in server_addr, client_addr;
    pthread_t tid;

    // Set up signal handler for Ctrl+C
    signal(SIGINT, handle_signal);

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("ERROR: Socket creation failed");
        return EXIT_FAILURE;
    }

    // Reuse address
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("ERROR: setsockopt failed");
        return EXIT_FAILURE;
    }

    // Bind socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: Bind failed");
        return EXIT_FAILURE;
    }

    // Start listening
    if (listen(server_socket, 10) < 0) {
        perror("ERROR: Listen failed");
        return EXIT_FAILURE;
    }

    printf("=== Chat server started on port %d ===\n", port);

    // Open chat history file
    chat_history = fopen("chat_history.txt", "a");
    if (!chat_history) {
        perror("ERROR: Could not open chat history file");
        return EXIT_FAILURE;
    }

    // Main accept loop
    while (1) {
        socklen_t client_len = sizeof(client_addr);
        int new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);

        if (new_socket < 0) {
            perror("ERROR: Accept failed");
            continue;
        }

        // Create new client struct
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = client_addr;
        client->socket = new_socket;

        add_client(client);

        // Create thread for the client
        if (pthread_create(&tid, NULL, &handle_client, (void *)client) != 0) {
            perror("ERROR: pthread_create failed");
            free(client);
        }
    }

    return EXIT_SUCCESS;
}
