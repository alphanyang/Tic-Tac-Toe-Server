#include "protocol.h"
#include "uthash.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
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
    // create hashmap for player names and socket numbers
    //struct Player *players;
    pthread_mutex_t mutex;
} GameState;


typedef struct {
    int socket;
    char name[50];
    UT_hash_handle hh;
} Player;

void *game_thread(void *arg);
void update_game_state(GameState* game_state, int player, const char* move);
int add_player(int socket, const char* name);
Player* find_player(int socket);
void remove_player(Player* p);
void free_players();

Player* players = NULL;

int main() {
    int server_socket, opt = 1;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    pthread_t thread;
    printf("Server address: %s\n", inet_ntoa(server_address.sin_addr));
    printf("Test: %s\n", format_message(MSG_NAME, "Joe Smith"));
    printf("Test: %s\n", format_message(MSG_WAIT));
    printf("Test: %s\n", format_message(MSG_MOVE, "2,2", "X"));
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

    // Initialize game board
    char initial_board[10] = {'.', '.', '.', '.', '.', '.', '.', '.', '.', '\0'};
    memcpy(game_state->board, initial_board, sizeof(initial_board));
    printf("Board: %s", game_state->board);

    // Receive and store player 1's name
    char player_name[50];
    char player_input[50];
    int recv_size = recv(game_state->player1_socket, player_input, sizeof(player_input), 0);
    int player_added;
    do {
        recv_size = recv(game_state->player1_socket, player_input, sizeof(player_input), 0);
        if (recv_size > 0) {
            player_input[recv_size] = '\0';
            sscanf(player_input, "PLAY %49s", player_name);
            player_added = add_player(game_state->player1_socket, player_name);
            if (!player_added) {
                send(game_state->player1_socket, "Name already exists, please choose another name.\n", 51, 0);
            }
        } else {
            perror("Error receiving player 1 name from client");
        }
    } while (!player_added);

    printf("Received player 1 name: %s\n", player_name);

    // Receive and store player 2's name
    do {
        recv_size = recv(game_state->player2_socket, player_input, sizeof(player_input), 0);
        if (recv_size > 0) {
            player_input[recv_size] = '\0';
            sscanf(player_input, "PLAY %49s", player_name);
            player_added = add_player(game_state->player2_socket, player_name);
            if (!player_added) {
                send(game_state->player2_socket, "Name already exists, please choose another name.\n", 51, 0);
            }
        } else {
            perror("Error receiving player 2 name from client");
        }
    } while (!player_added);

    printf("Received player 2 name: %s\n", player_name);

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
        update_game_state(game_state, 1, move);

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

void update_game_state(GameState* game_state, int player, const char* move) {
    // Parse the move and update the game state's board
    int position = atoi(move) - 1;
    char mark = (player == 1) ? 'X' : 'O';

    if (position >= 0 && position < 9 && game_state->board[position] == '.') {
        game_state->board[position] = mark;
    } else {
        // Handle invalid move
        printf("Invalid move from player %d\n", player);
    }
}

// Add a player to the hashmap, returns 1 if added successfully, 0 if name already exists
int add_player(int socket, const char* name) {
    Player* p;

    // Check if name already exists
    for (p = players; p != NULL; p = p->hh.next) {
        if (strcmp(p->name, name) == 0) {
            return 0;  // Name already exists
        }
    }

    // Add the new player
    p = malloc(sizeof(Player));
    p->socket = socket;
    snprintf(p->name, sizeof(p->name), "%s", name);
    HASH_ADD_INT(players, socket, p);

    return 1;  // Player added successfully
}
// Find a player in the hashmap by socket
Player* find_player(int socket) {
    Player* p;
    HASH_FIND_INT(players, &socket, p);
    return p;
}

// Remove a player from the hashmap
void remove_player(Player* p) {
    HASH_DEL(players, p);
    free(p);
}

// Free the hashmap
void free_players() {
    Player* current_player, *tmp;
    HASH_ITER(hh, players, current_player, tmp) {
        HASH_DEL(players, current_player);
        free(current_player);
    }
}
