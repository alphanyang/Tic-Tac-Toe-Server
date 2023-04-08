#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "protocol.h"

#define _XOPEN_SOURCE 700
#define PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

/*
    This is the main server file. It contains the main loop and
    the signal handler.
*/

// typedef struct GameState {
//     char board[10];
//     char current_player;
//     int player1_socket;
//     int player2_socket;
// } GameState;

//GameState game_state;

int server_socket;

void initialize_server(int port);
void *connection_handler(void *arg);
void setup_signal_handlers();
void accept_connections();
void print_game_state();
void process_client_messages(int client_socket);
void send_server_messages(int client_socket);

int main() {
    // if (argc != 2) {
    //     fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    //     exit(1);
    // }
    
    setup_signal_handlers();

    // Initialize the server and bind to the specified port
    initialize_server(PORT);

    // Main server loop
    while (1) {
        // Accept new connections and create a game session
        accept_connections();
        //print_game_state();
    }

    // Close the server
    close(server_socket);
    return 0;
}

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\nReceived signal %d. Shutting down the server...\n", signum);
        close(server_socket);
        exit(0);
    } else if (signum == SIGUSR1) {
        printf("\nReceived signal %d. Printing game state...\n", signum);
        lock_game_state();
        print_game_state();
        unlock_game_state();
    } else if (signum == SIGUSR2) {
        printf("\nReceived signal %d. Resetting game state...\n", signum);
        lock_game_state();
        reset_game_state();
        unlock_game_state();
    }
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);  // Handle SIGINT (Ctrl+C)
    sigaction(SIGTERM, &sa, NULL); // Handle SIGTERM (termination request)
    sigaction(SIGUSR1, &sa, NULL); // Handle SIGUSR1 (print game state)
    sigaction(SIGUSR2, &sa, NULL); // Handle SIGUSR2 (reset game state)
}

void initialize_server(int port) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_socket, 5) < 0) {
        perror("listen");
        exit(1);
    }
}

void accept_connections() {
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
    if (client_socket < 0) {
        perror("accept");
        return;
    }

    printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, connection_handler, (void *)&client_socket) < 0) {
        perror("pthread_create");
        return;
    }
}


void *connection_handler(void *arg) {
    int client_socket = *(int *)arg;

    // Test: send a sample game state message to the client
    // char *sample_msg = "STATE|1|2|1|XOXOXOXOX";
    // send_message(client_socket, sample_msg);

    // Test: send a welcome message to the client
    char *welcome_msg = "WELCOME|7|Welcome|";
    send_message(client_socket, welcome_msg);

    while (1) {
        char *message = receive_message(client_socket);
        if (message == NULL) {
            break;
        }

        // Process move command
        char piece;
        int row, col;
        if (sscanf(message, "MOVE|%c|%d,%d", &piece, &row, &col) == 3) {
            // Update game state here based on the move (e.g., game_state.board)

            // Send the updated game state to the client
            char response[64];
            snprintf(response, sizeof(response), "MOVD|%d|%c|%d,%d|....X....", client_socket, piece, row, col);
            send_message(client_socket, response);
        } else {
            printf("Invalid message format: %s\n", message);
        }

        free(message);
    }

    // Process incoming messages from clients, update the game state, and handle any necessary game logic
    process_client_messages(client_socket);

    // Send appropriate messages back to clients based on game state, client requests, or game events
    send_server_messages(client_socket);

    close(client_socket);
    return NULL;
}

void process_client_messages(int client_socket) {
    char *message = receive_message(client_socket);
    if (message != NULL) {
        // Process the message and update the game state
        // You may want to create a function to handle each specific message type
        char cmd[5];
        char move[10];
        char player_role;
        int row, col;
        sscanf(message, "%4[^|]|%c|%d,%d", cmd, &player_role, &row, &col);
        sprintf(move, "%d,%d", row, col);
        
        if (strcmp(cmd, "MOVE") == 0) {
            if (process_move(player_role, row, col)) {
                // If the move is valid, update the game state and send the updated state to the client
                char move_confirmation[64];
                snprintf(move_confirmation, sizeof(move_confirmation), "MOVD|%c|%d,%d|%s", player_role, row, col, game_state_string());
                send_message(client_socket, move_confirmation);
            } else {
                // If the move is invalid, send an error message to the client
                char move_error[64];
                // snprintf(move_error, sizeof(move_error), "ERRM|%c|%d,%d|Invalid move", player_role, row, col);
                snprintf(move_error, sizeof(move_error), "INVL|24|That space is occupied.|");
                send_message(client_socket, move_error);
            }
        }
        
        free(message);
    }
}

void send_server_messages(int client_socket) {
    // Retrieve the current game state
    lock_game_state();
    char current_player = get_current_player();
    const char *board = get_board();
    int player1_socket = get_player_socket('X');
    int player2_socket = get_player_socket('O');
    unlock_game_state();

    // Check if the game is over
    char result = game_over(board);
    if (result != '\0') {
        char *over_msg;
        if (result == 'D') {
            over_msg = format_message(MSG_OVER, "D", "The game is a draw.");
        } else {
            over_msg = format_message(MSG_OVER, result == 'X' ? "W" : "L", "Player has won the game.");
        }

        send_message(player1_socket, over_msg);
        send_message(player2_socket, over_msg);
        free(over_msg);
        return;
    }

    // Send a message to the current player to make their move
    if (client_socket == (current_player == 'X' ? player1_socket : player2_socket)) {
        char *move_msg = format_message(MSG_MOVE, current_player, "");
        send_message(client_socket, move_msg);
        free(move_msg);
    }

    // Send the updated board state to both players
    char *movd_msg = format_message(MSG_MOVD, current_player, "", board);
    send_message(player1_socket, movd_msg);
    send_message(player2_socket, movd_msg);
    free(movd_msg);
}