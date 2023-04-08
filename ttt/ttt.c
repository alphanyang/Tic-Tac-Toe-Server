#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "protocol.h"

#define PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

typedef struct {
    int player_id;
    int player_count;
    int current_turn;
    char board[9];
} GameState;

GameState game_state;

int client_socket;

void connect_to_server(const char *domain_name, int port);
void handle_server_messages();
void get_player_input_and_send();
void init_game_state();

int main() {
    // if (argc != 3) {
    //     fprintf(stderr, "Usage: %s <domain_name> <port>\n", argv[0]);
    //     exit(1);
    // }

    // Establish a connection with the server
    init_game_state();
    connect_to_server(SERVER_ADDRESS, PORT);

    // Main game loop
    while (1) {
        // Process server messages and update the game state
        handle_server_messages();

        // Get player input and send it to the server
        get_player_input_and_send();
    }

    // Close the connection
    close(client_socket);
    return 0;
}

void init_game_state() {
    game_state.player_id = -1;
    game_state.player_count = 0;
    game_state.current_turn = -1;
    memset(game_state.board, ' ', 9);
}


void connect_to_server(const char *domain_name, int port) {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(domain_name);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server: %s\n", inet_ntoa(server_addr.sin_addr));
}

void process_server_message(const char *message) {
    //char board[10]; // Leave room for a null terminator
    printf("%s\n", message);

    // if (sscanf(message, "STATE|%d|%d|%d|%9s", &player_id, &player_count, &current_turn, board) == 4) {
    //     game_state.player_id = player_id;
    //     game_state.player_count = player_count;
    //     game_state.current_turn = current_turn;
    //     memcpy(game_state.board, board, 9);
    // } else {
    //     printf("Invalid message format: %s\n", message);
    // }
}

void handle_server_messages() {
    char *message = receive_message(client_socket);
    if (message != NULL) {
        printf("Received message from server: %s\n", message);
        process_server_message(message);
        free(message);
    }
}

//inputs
void get_player_input_and_send() {
    char input[64];
    printf("Enter your move (e.g., MOVE X 2,2): ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        perror("fgets");
        return;
    }
    input[strcspn(input, "\n")] = 0; // Remove newline character

    char cmd[5];
    char player_role;
    char move[10];
    int row, col;
    sscanf(input, "%4s %c %d,%d", cmd, &player_role, &row, &col);
    sprintf(move, "%d,%d", row, col);

    if (strcmp(cmd, "MOVE") == 0 && validate_move(move)) {
        // Format the message as a MOVE message and send it to the server
        char move_msg[64];
        snprintf(move_msg, sizeof(move_msg), "%s|%c|%d,%d", cmd, player_role, row, col);
        send_message(client_socket, move_msg);
    } else {
        printf("Invalid move format. Please enter your move as 'MOVE X row,column' (e.g., MOVE X 2,2)\n");
    }
}


