#include "protocol.h"
#include "uthash.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#define SERVER_PORT 5037
#define MAX_CLIENTS 64

typedef struct {
    int player1_socket;
    int player2_socket;
    char player1_name[50];
    char player2_name[50];
    char board[10];
    pthread_mutex_t mutex;
} GameState;

typedef struct {
    int socket;
    char name[50];
    UT_hash_handle hh;
} Player;

typedef struct Client {
    int socket;
    Player *player;
    GameState *game_state;
    struct Client *next;
} Client;

void *game_thread(void *arg);
void update_game_state(GameState* game_state, int player, const char* move);
int add_player(int socket, const char* name);
Player* find_player(int socket);
void remove_player(Player* p);
void free_players();
void *client_thread(void *arg);
void add_client(Client *new_client);
void remove_client(Client *client);
void print_clients();

Player* players = NULL;
Client *clients_head = NULL;

int connected_players = 0;
pthread_cond_t players_connected_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t players_connected_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Add the global variables for the list of GameState instances
GameState **game_state_list = NULL;
int game_state_list_length = 0;
pthread_mutex_t game_state_list_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
    int server_socket, opt = 1;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    pthread_t thread;
    //printf("Test: %s\n", format_message(MSG_BEGN, "Joe Smith", "X"));
    // printf("Server address: %s\n", inet_ntoa(server_address.sin_addr));
    // printf("Test: %s\n", format_message(MSG_NAME, "Joe Smith"));
    // printf("Test: %s\n", format_message(MSG_WAIT));
    // printf("Test: %s\n", format_message(MSG_MOVE, "2,2", "X"));

    // Message *testMsg = parse_message(format_message(MSG_MOVE, "2,2", "X"));
    // printf("Test: %s\n", testMsg->msg_type);
    // printf("Test: %s\n", testMsg->content);

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

    signal(SIGPIPE, SIG_IGN);

    while (1) {
        // Accept incoming connections
        int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Error accepting connection from client");
            continue;
        }

        // Create new client struct and add to clients list
        Client *new_client = (Client*) malloc(sizeof(Client));
        new_client->socket = client_socket;
        new_client->next = NULL;
        add_client(new_client);

        // Print connected clients and their socket numbers
        print_clients(); // Add this line

        // Create a separate thread to handle the client
        pthread_t client_thread_id;
        if (pthread_create(&client_thread_id, NULL, client_thread, new_client) != 0) {
            perror("Error creating client thread");
            exit(EXIT_FAILURE);
        }

        // Detach client thread
        if (pthread_detach(client_thread_id) != 0) {
            perror("Error detaching client thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < game_state_list_length; i++) {
        free(game_state_list[i]);
    }
    free(game_state_list);
    // Close server socket
    close(server_socket);
}

void *client_thread(void *arg) {
    Client *client = (Client*) arg;
    Player *player = (Player *) malloc(sizeof(Player));
    memset(player->name, 0, sizeof(player->name)); // Initialize name to an empty string
    client->player = player;

    pthread_mutex_lock(&game_state_list_mutex);

    if (game_state_list == NULL || game_state_list_length % 2 == 0) {
        GameState *new_game_state = (GameState *) malloc(sizeof(GameState));
        new_game_state->player1_socket = -1;
        new_game_state->player2_socket = -1;
        game_state_list_length++;
        game_state_list = (GameState **) realloc(game_state_list, game_state_list_length * sizeof(GameState *));
        game_state_list[game_state_list_length - 1] = new_game_state;
    }

    GameState *current_game_state = game_state_list[game_state_list_length - 1];
    client->game_state = current_game_state;

    pthread_mutex_unlock(&game_state_list_mutex);

    int player_socket = client->socket;
    int player_added = 0;
    do {
        printf("%d", client->game_state->player1_socket);
        char *msg = receive_message(player_socket);
        if (client->game_state->player1_socket == -1 && client->game_state->player2_socket != player_socket) {
            // Player 1 is the first player to connect, assign them the 'X' role
            printf("added p1\n");
            client->game_state->player1_socket = player_socket;
            printf("player1 socket: %d\n", client->game_state->player1_socket);
            printf("player2 socket: %d\n", client->game_state->player2_socket);
            snprintf(client->game_state->player1_name, sizeof(client->game_state->player1_name), "%s", msg);
        } else if (client->game_state->player2_socket != player_socket) {
            // Player 2 is the second player to connect, assign them the 'O' role
            printf("added p2");
            client->game_state->player2_socket = player_socket;
            printf("player1 socket: %d\n", client->game_state->player1_socket);
            printf("player2 socket: %d\n", client->game_state->player2_socket);
            snprintf(client->game_state->player2_name, sizeof(client->game_state->player2_name), "%s", msg);
        }
        snprintf(client->player->name, sizeof(client->player->name), "%s", msg);
        send_message(player_socket, format_message(MSG_NAME, client->player->name));
        player_added = add_player(player_socket, msg);
    } while (player_added == 0);


    printf("%s\n", client->game_state->player1_name);
    printf("%s\n", client->game_state->player2_name);

    // Update the number of connected players and signal when both players are connected
    pthread_mutex_lock(&players_connected_mutex);
    connected_players++;
    if (connected_players == 2) {
        pthread_cond_signal(&players_connected_cond);
    }
    pthread_mutex_unlock(&players_connected_mutex);

    // Wait for both players to connect before starting the game
    pthread_mutex_lock(&players_connected_mutex);
    while (connected_players < 2) {
        pthread_cond_wait(&players_connected_cond, &players_connected_mutex);
    }
    pthread_mutex_unlock(&players_connected_mutex);

    // Both players are connected, create the game_thread to manage the game
    pthread_t game_thread_id;
    if (pthread_create(&game_thread_id, NULL, game_thread, client->game_state) != 0) {
        perror("Error creating game thread");
        exit(EXIT_FAILURE);
    }

    // Clean up and close the client socket
    close(client->socket);
    remove_client(client);
    free(client->game_state);
    free_players(); // Added this line
    free(client);

    return NULL;
}

void *game_thread(void *arg) {
    GameState *game_state = (GameState *) arg;

    // Initialize the game board
    char initial_board[10] = {'.', '.', '.', '.', '.', '.', '.', '.', '.', '\0'};
    memcpy(game_state->board, initial_board, sizeof(initial_board));
    printf("%d", game_state->player1_socket);
    printf("%d", game_state->player2_socket);

    // Send begin messages to both players
    send_message(game_state->player1_socket, format_message(MSG_BEGN, "X", game_state->player2_name));
    send_message(game_state->player2_socket, format_message(MSG_BEGN, "O", game_state->player1_name));

    // // Main game loop
    while (!game_over(game_state->board)) {

    }
    //     int active_socket, inactive_socket;
    //     char active_mark;

    //     // if (game_state->turn == 1) {
    //     //     active_socket = game_state->player1_socket;
    //     //     inactive_socket = game_state->player2_socket;
    //     //     active_mark = 'X';
    //     //     game_state->turn = 2;
    //     // } else {
    //     //     active_socket = game_state->player2_socket;
    //     //     inactive_socket = game_state->player1_socket;
    //     //     active_mark = 'O';
    //     //     game_state->turn = 1;
    //     // }

    //     // Request a move from the active player
    //     //send_message(active_socket, format_message(MSG_MOVE, NULL, NULL));

    //     // Receive and process the move
    //     //char *move = receive_message(active_socket);
    //     //update_game_state(game_state, game_state->turn, move);

    //     // Send the updated board to both players
    //     //send_message(active_socket, format_message(MSG_BOARD, game_state->board, active_mark));
    //     //send_message(inactive_socket, format_message(MSG_BOARD, game_state->board, active_mark));

    //     // Check if the game is over
    //     // if (game_over(game_state->board)) {
    //     //     // Notify players of the game result
    //     //     if (game_won(game_state->board)) {
    //     //         //send_message(active_socket, format_message(MSG_WIN, NULL, NULL));
    //     //         //send_message(inactive_socket, format_message(MSG_LOSE, NULL, NULL));
    //     //     } else {
    //     //         //send_message(active_socket, format_message(MSG_DRAW, NULL, NULL));
    //     //         //send_message(inactive_socket, format_message(MSG_DRAW, NULL, NULL));
    //     //     }
    //     // }
    // }

    // Clean up the game state
    close(game_state->player1_socket);
    close(game_state->player2_socket);
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

void add_client(Client *new_client) {
    pthread_mutex_lock(&clients_mutex);
    new_client->next = clients_head;
    clients_head = new_client;
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(Client *client) {
    pthread_mutex_lock(&clients_mutex);
    if (clients_head == client) {
        clients_head = client->next;
    } else {
        Client *current = clients_head;
        while (current->next != NULL) {
            if (current->next == client) {
                current->next = client->next;
                break;
            }
            current = current->next;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Add print_clients function
void print_clients() {
    pthread_mutex_lock(&clients_mutex);
    printf("Connected clients (socket numbers): ");
    Client *current = clients_head;
    while (current != NULL) {
        printf("%d ", current->socket);
        current = current->next;
    }
    printf("\n");
    pthread_mutex_unlock(&clients_mutex);
}