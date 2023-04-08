#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5037
#define MAX_MESSAGE_LENGTH 1024

int connect_to_server(const char *ip, int port);
void send_message(int sockfd, const char *msg);
char *receive_message(int sockfd);
int read_user_input(char *input);

typedef struct {
    int player_id;
    // Add any other game-specific state here
} game_state_t;

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 2 && argc != 1) {
        printf("Usage: %s [server_ip_address]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = (argc == 2) ? argv[1] : SERVER_IP;
    int client_socket = connect_to_server(ip, SERVER_PORT);

    // Parse server IP address
    // struct sockaddr_in server_address;
    // memset(&server_address, 0, sizeof(server_address));
    // server_address.sin_family = AF_INET;
    // server_address.sin_port = htons(8888);
    // if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) != 1) {
    //     perror("Error parsing server IP address");
    //     exit(EXIT_FAILURE);
    // }

    // // Create socket
    // int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    // if (client_socket == -1) {
    //     perror("Error creating socket");
    //     exit(EXIT_FAILURE);
    // }

    // // Connect to server
    // if (connect(client_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
    //     perror("Error connecting to server");
    //     exit(EXIT_FAILURE);
    // }

    // Wait for game to start
    game_state_t game_state;
    if (recv(client_socket, &game_state, sizeof(game_state_t), 0) == -1) {
        perror("Error receiving game state from server");
        exit(EXIT_FAILURE);
    }

    // Main game loop
    while (1) {
        // Get player name from command-line argument
        char* player_name = argv[1];

        // Send player name to server
        if (send(client_socket, player_name, strlen(player_name) + 1, 0) == -1) {
            perror("Error sending player name to server");
            break;
        }

        // Receive game state from server
        if (recv(client_socket, &game_state, sizeof(game_state_t), 0) == -1) {
            perror("Error receiving game state from server");
            break;
        }

        // Print game state
        // Add any other game-specific print statements here

        // Get player's move
        char move[10];
        printf("Enter your move: ");
        if(fgets(move, 10, stdin) == NULL) {
            perror("Error reading player move");
            break;
        }

        // Send player's move to server
        if (send(client_socket, move, strlen(move) + 1, 0) == -1) {
            perror("Error sending move to server");
            break;
        }
    }

    // Close connection to server
    close(client_socket);

    return 0;
}

int connect_to_server(const char *ip, int port) {
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set server address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_address.sin_addr) != 1) {
        perror("Error parsing server IP address");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int read_user_input(char *input) {
    // Read a line of input from the user
    // Return the number of characters read
    // Return -1 if there was an error
    if (fgets(input, MAX_MESSAGE_LENGTH, stdin) == NULL) {
        perror("Error reading user input");
        return -1;
    }

    return 0;
}
