#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define SERVER_PORT 5037
#define MAX_CLIENTS 64

typedef struct {
    int player1_socket;
    int player2_socket;
    char board[10];
    char current_player;
    pthread_mutex_t mutex;
} GameState;

void *game_thread(void *arg);
void player_thread(void *arg);


int main() {
    int server_socket, opt = 1;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    pthread_t thread;
    printf("Server address: %s\n", inet_ntoa(server_address.sin_addr));

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
    server_address.sin_port = htons(SERVER_PORT);
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    while (1) {
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

        pthread_mutex_init(&game_state->mutex, NULL);

        if (pthread_create(&thread, NULL, game_thread, game_state) != 0) {
            perror("Error creating game thread");
            exit(EXIT_FAILURE);
        }

        // Detach game thread
        if (pthread_detach(thread) != 0) {
            perror("Error detaching game thread");
            exit(EXIT_FAILURE);
        }
    }

    // Close server socket
    close(server_socket);
}

void* game_thread(void* arg) {
    GameState* game_state = (GameState*) arg;

    // Initialize the game state
    // Implement game initialization logic here
    // ...
    char initial_board[10] = {'.', '.', '.', '.', '.', '.', '.', '.', '.', '.'};
    memcpy(game_state->board, initial_board, sizeof(initial_board));

    // Send initial game state to players
    pthread_mutex_lock(&game_state->mutex);
    send(game_state->player1_socket, game_state, sizeof(GameState), 0);
    send(game_state->player2_socket, game_state, sizeof(GameState), 0);
    pthread_mutex_unlock(&game_state->mutex);

    // Game loop
    while (1) {
        // Receive move from player 1
        char move[10];
        
        if (recv(game_state->player1_socket, move, sizeof(move), 0) == -1) {
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
        if (recv(game_state->player2_socket, move, sizeof(move), 0) == -1) {
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
    pthread_mutex_destroy(&game_state->mutex);
    free(game_state);

    return NULL;
}

void player_thread(void *arg) {
    int sockfd = *((int *) arg);
    GameState game_state_copy;
    char move[10];
    while (1) {
    // Receive move from player
    if (recv(sockfd, move, 10, 0) == -1) {
        perror("Error receiving move from player");
        break;
    }

    // Lock game state mutex and update game state
    pthread_mutex_lock(&game_state_copy.mutex);
    //memcpy(&game_state_copy, (GameState*) &move, sizeof(GameState));
    memcpy(game_state_copy.board, move, sizeof(game_state_copy.board));
    pthread_mutex_unlock(&game_state_copy.mutex);

    // Send updated game state to other player
    int other_sockfd = (sockfd == game_state_copy.player1_socket) ? game_state_copy.player2_socket : game_state_copy.player1_socket;
        if (send(other_sockfd, move, 10, 0) == -1) {
            perror("Error sending game state to other player");
            break;
        }
    }

    close(sockfd);
}

