#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int client_sockets[2] = {-1, -1}; // Store two client socket
int server_socket;

void handle_sigint(int sig) {
    printf("\nServer is shutting down gracefully...\n");
    close(client_sockets[0]);
    close(client_sockets[1]);
    close(server_socket);
    exit(0);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];

    signal(SIGINT, handle_sigint); // Capture Ctrl+C to shut down the server properly

    // create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

   // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(1);
    }

    // listen for cleint connection
    if (listen(server_socket, 2) < 0) {
        perror("Listening failed");
        exit(1);
    }

    printf("Server started, waiting for client connections...\n");

    for (int i = 0; i < 2; i++) {
        addr_size = sizeof(client_addr);
        client_sockets[i] = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sockets[i] < 0) {
            perror("Connection acceptance failed");
            exit(1);
        }
        printf("Client %d connected\n", i + 1);
    }

    printf("Both clients connected, chat can begin.\n");

    while (1) {
    for (int i = 0; i < 2; i++) {
        if (client_sockets[i] == -1) continue; //Skip if the client is disconnected

        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sockets[i], buffer, BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) {
            printf("Client %d disconnected\n", i + 1);
            close(client_sockets[i]);
            client_sockets[i] = -1; // Mark as invalid
            continue;
        }

        printf("Client %d: %s\n", i + 1, buffer);

       // If the client sends "BYE", shut down the server
        if (strncmp(buffer, "BYE", 3) == 0) {
            printf("Client %d ended the chat, server shutting down.\n", i + 1);
            close(client_sockets[0]);
            close(client_sockets[1]);
            close(server_socket);
            exit(0);
        }

     // Ensure the target client is still connected
        if (client_sockets[1 - i] != -1) {
            int total_sent = 0;
            int len = strlen(buffer);
            while (total_sent < len) {
                int sent = send(client_sockets[1 - i], buffer + total_sent, len - total_sent, 0);
                if (sent == -1) {
                    perror("send failed");
                    break;
                }
                total_sent += sent;
            }
        } else {
            printf("Message could not be forwarded as Client %d is disconnected.\n", 2 - i);
        }
    }
}
}
