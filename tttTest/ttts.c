#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define MAX_CLIENTS 64

typedef struct {
    int player1_socket;
    int player2_socket;
    char board[10];
    char current_player;
} GameState;

void *handle_game(void *arg);
void send_message(int sockfd, const char *msg);
char *receive_message(int sockfd);
void *game_thread(void *arg);

int main() {
    int server_socket, client_socket, opt = 1;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    pthread_t thread;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    // Bind server socket to address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8888);
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    // Accept two incoming connections and create game thread
    GameState* game_state = malloc(sizeof(GameState));
    game_state->player1_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_len);
    if (game_state->player1_socket == -1) {
        perror("Error accepting connection from player 1");
        exit(EXIT_FAILURE);
    }

    game_state->player2_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_len);
    if (game_state->player2_socket == -1) {
        perror("Error accepting connection from player 2");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread, NULL, game_thread, game_state) != 0) {
        perror("Error creating game thread");
        exit(EXIT_FAILURE);
    }

    // Wait for game thread to finish
    if (pthread_join(thread, NULL) != 0) {
        perror("Error joining game thread");
        exit(EXIT_FAILURE);
    }
    
    // Close server socket
    close(server_socket);
}

void* game_thread(void* arg) {
    GameState* game_state = (GameState*) arg;

    // Game loop
    while (1) {
        // Receive move from player 1
        char move[10];
        if (recv(game_state->player1_socket, move, 10, 0) == -1) {
            perror("Error receiving move from player 1");
            break;
        }

        // Update game state based on player 1's move

        // Send updated game state to player 2
        if (send(game_state->player2_socket, game_state, sizeof(GameState), 0) == -1) {
            perror("Error sending game state to player 2");
            break;
        }

        // Receive move from player 2
        if (recv(game_state->player2_socket, move, 10, 0) == -1) {
            perror("Error receiving move from player 2");
            break;
        }

        // Update game state based on player 2's move

        // Send updated game state to player 1
        if (send(game_state->player1_socket, game_state, sizeof(GameState), 0) == -1) {
            perror("Error sending game state to player 1");
            break;
        }
    }

    // Clean up game state and close sockets
    close(game_state->player1_socket);
    close(game_state->player2_socket);
    free(game_state);

    return NULL;
}

void *handle_game(void *arg) {
    // Get the socket file descriptors for the two players
    // Allocate memory for the game state
    // Initialize the game state
    // Send the initial game state to the players
    // Main game loop
    // Send the game over message to the players
    // Close the socket file descriptors
    // Free the memory allocated for the game state
    return NULL;
}

void send_message(int sockfd, const char *msg) {
    // Send the message to the client
}

char *receive_message(int sockfd) {
    // Receive a message from the client
    // Return the message
}
