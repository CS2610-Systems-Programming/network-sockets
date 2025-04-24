#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 2048

//global variables
int client_socket;
char username[50];

//function to handle receiving messages from server
void *receive_handler(void *arg)
{
        (void) arg;

        char message[BUFFER_SIZE];

        while(1)
        {
                int receive = recv(client_socket, message, BUFFER_SIZE, 0);
                if (receive > 0)
                {
                        //print the message
                        printf("%s", message);
                        fflush(stdout); //ensure output is shown immediately
                }
                else if (receive == 0)
                {
                        printf("Server disconnected.\n");
                        break;
                }
                else
                {
                        //an error occured
                        perror("ERROR: Failed to receive message from server");
                        break;
                }

                //clear the buffer
                memset(message, 0, BUFFER_SIZE);
        }

        //if we break out of the loop, the connection has been closed
        close(client_socket);
        exit(0);

        return NULL;
}

//function to handle sending message to server
void send_handler()
{
        char message[BUFFER_SIZE];
        char buffer[BUFFER_SIZE + 50]; //extra space for username

        while(1)
        {
                //get the user input
                fgets(message, BUFFER_SIZE, stdin);

                //check if user wants to exit
                if (strcmp(message, "exit\n") == 0)
                {
                        //send exit message to server
                        send(client_socket, message, strlen(message), 0);
                }

                //send message to server
                if (send(client_socket, message, strlen(message), 0) < 0)
                {
                        perror("ERROR: Failed to send message to server");
                        break;
                }

                //clear the buffers
                memset(message, 0, BUFFER_SIZE);
                memset(buffer, 0, BUFFER_SIZE + 50);
        }

        //close socket and exit
        close(client_socket);
        exit(0);
}

//funtion to handle signals for graceful shutdown
void handle_signal (int sig)
{
        if (sig == SIGINT)
        {
                printf("\nDisconnecting from server...\n");
                close(client_socket);
                exit(0);
        }
}

int main(int argc, char *argv[])
{
        if (argc != 3)
        {
                printf("Usage: %s <ip> <port>\n", argv[0]);
                return EXIT_FAILURE;
        }

        char *ip = argv[1];
        int port = atoi(argv[2]);

        //register signal handler
        signal(SIGINT, handle_signal);

        //create socket
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0)
        {
                perror("ERROR: Socket creation failed");
                return EXIT_FAILURE;
        }

        //configure server address
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip);

        //connect to server
        if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
                perror("ERROR: Connection failed");
                return EXIT_FAILURE;
        }

        //get username
        printf("Enter your username: ");
        fgets(username, 50, stdin);
        username[strcspn(username, "\n")] = '\0'; //remove trailing newline

        //send username to server
        if (send(client_socket, username, strlen(username), 0) < 0)
        {
                perror("ERROR: Failed to send username");
                return EXIT_FAILURE;
        }

        printf("=== WELCOME TO THE CHAT ===\n");

        //create thread for receiving messages
        pthread_t recv_thread;
        if (pthread_create(&recv_thread, NULL, &receive_handler, NULL) != 0)
        {
                perror("ERROR: Failed to create thread");
                return EXIT_FAILURE;
        }

        //detach the receive thread so it cleans up after itself
        pthread_detach(recv_thread);

        //handle sending messages
        send_handler();

        return EXIT_SUCCESS;
}