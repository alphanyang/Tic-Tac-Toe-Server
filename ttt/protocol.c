#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

/*
3 Protocol
The game protocol involves nine message types:
• PLAY name
• WAIT
• BEGN role name
• MOVE role position
• MOVD role position board
• INVL reason
• RSGN
• DRAW message
• OVER outcome reason
Figure 1 shows an example of communication between the server and two clients during the
set-up and first two moves of a game. Figure 2 shows two proposed draws, the first rejected and the
second accepted.
3.1 Message format
Messages are broken into fields. A field consists of one or more characters, followed by a vertical bar.
The first field is always a four-character code. The second field gives the length of the remaining
message in bytes, represented as string containing a decimal integer in the range 0–255. This length
does not include the bar following the size field, so a message with no additional fields, such as
WAIT, will give its size as 0. (Note that this documentation will generally omit the length field
when discussing messages for simplicity.) Subsequent fields are variable-length strings.
Note that a message will always end with a vertical bar. Implementations must use this to detect
improperly formatted messages.
The field types are:
name Arbitrary text representing a player’s name.
role Either X or O.
position Two integers (1, 2, or 3) separated by a comma.
board Nine characters representing the current state of the grid, using a period (.) for unclaimed
grid cells, and X or O for claimed cells.
reason Arbitrary text giving why a move was rejected or the game has ended.
message One of S (suggest), A (accept), or R (reject).
outcome One of W (win), L (loss), or D (draw).
NAME|10|Joe Smith|
WAIT|0|
MOVE|6|X|2,2|
MOVD|16|X|2,2|....X....|
INVL|24|That space is occupied.|
DRAW|2|S|
OVER|26|W|Joe Smith has resigned.|
*/

#define BUFFER_SIZE 512

typedef struct {
    char board[10];
    char current_player;
    int player1_socket;
    int player2_socket;
} GameState;

static GameState game_state;
pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *get_board() {
    return game_state.board;
}

char get_current_player() {
    return game_state.current_player;
}

void send_message(int sockfd, const char *msg) {
    if (send(sockfd, msg, strlen(msg), 0) == -1) {
        perror("send");
    }
}

char *receive_message(int sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
        return NULL;
    }

    buffer[bytes_received] = '\0';
    char *msg = (char *)malloc(bytes_received + 1);
    strcpy(msg, buffer);
    return msg;
}

Message *parse_message(const char *msg) {
    Message *message = (Message *)malloc(sizeof(Message));
    char *msg_copy = (char *)malloc(strlen(msg) + 1);
    strcpy(msg_copy, msg);

    char *token = strtok(msg_copy, "|");
    message->msg_type = strdup(token);
    token = strtok(NULL, "|");
    message->size = atoi(token);
    message->content = strdup(msg + strlen(message->msg_type) + 1 + strlen(token) + 1);
    free(msg_copy);

    return message;
}

char *format_message(const char *msg_type, ...) {
    char *formatted_msg;
    char content[BUFFER_SIZE];
    va_list args;
    va_start(args, msg_type);

    if (strcmp(msg_type, MSG_PLAY) == 0) {
        snprintf(content, BUFFER_SIZE, "%s|", va_arg(args, char *));
    } else if (strcmp(msg_type, MSG_MOVE) == 0) {
        snprintf(content, BUFFER_SIZE, "%s|%s|", va_arg(args, char *), va_arg(args, char *));
    } // Add more cases for different message types as needed.

    va_end(args);

    int size = strlen(content);
    formatted_msg = (char *)malloc(strlen(msg_type) + 1 + 3 + 1 + size + 1);
    snprintf(formatted_msg, strlen(msg_type) + 1 + 3 + 1 + size + 1, "%s|%03d|%s", msg_type, size, content);

    return formatted_msg;
}

int validate_move(const char *move) {
    // Check if the move has the correct format and is within the valid range
    int row, col;
    if (sscanf(move, "%d,%d", &row, &col) != 2) {
        return 0;
    }
    if (row < 1 || row > 3 || col < 1 || col > 3) {
        return 0;
    }
    return 1;
}

char game_over(const char *board) {
    // Check rows, columns, and diagonals for a winner
    const int winning_patterns[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // Rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // Columns
        {0, 4, 8}, {2, 4, 6}            // Diagonals
    };

    for (int i = 0; i < 8; ++i) {
        if (board[winning_patterns[i][0]] == board[winning_patterns[i][1]] &&
            board[winning_patterns[i][1]] == board[winning_patterns[i][2]] &&
            (board[winning_patterns[i][0]] == 'X' || board[winning_patterns[i][0]] == 'O')) {
            return board[winning_patterns[i][0]];
        }
    }

    // Check for a draw (no empty spaces on the board)
    int is_draw = 1;
    for (int i = 0; i < 9; ++i) {
        if (board[i] == '.') {
            is_draw = 0;
            break;
        }
    }

    if (is_draw) {
        return 'D';
    }

    // The game is not over yet
    return '\0';
}

void update_board(char *board, const char *move, char *role) {
    // Update the board based on the given move and role
    int row, col;
    sscanf(move, "%d,%d", &row, &col);
    int index = (row - 1) * 3 + (col - 1);
    board[index] = role[0];
}


void print_game_state() {
    printf("Current game state:\n");
    printf("Board: %s\n", game_state.board);
    printf("Current player: %c\n", game_state.current_player);
}

void reset_game_state() {
    strcpy(game_state.board, ".........");
    game_state.current_player = 'X';
}

void lock_game_state() {
    pthread_mutex_lock(&game_state_mutex);
}

void unlock_game_state() {
    pthread_mutex_unlock(&game_state_mutex);
}


char *game_state_string() {
    char *game_state_str = (char *)malloc(10 * sizeof(char));
    strncpy(game_state_str, game_state.board, 9);
    game_state_str[9] = '\0';
    return game_state_str;
}

int process_move(char player_role, int row, int col) {
    int index = (row - 1) * 3 + (col - 1);

    lock_game_state(); // Lock the game state before making changes

    // Check if the cell is empty and the player role matches the current player
    if (game_state.board[index] == '.' && game_state.current_player == player_role) {
        game_state.board[index] = player_role;
        game_state.current_player = (player_role == 'X') ? 'O' : 'X'; // Toggle the current player

        unlock_game_state(); // Unlock the game state after making changes
        return 1; // Move was successful
    }

    unlock_game_state(); // Unlock the game state if the move was unsuccessful
    return 0; // Move was unsuccessful
}

void set_player_socket(char role, int socket) {
    lock_game_state();
    if (role == 'X') {
        game_state.player1_socket = socket;
    } else if (role == 'O') {
        game_state.player2_socket = socket;
    }
    unlock_game_state();
}

void set_current_player(char role) {
    lock_game_state();
    game_state.current_player = role;
    unlock_game_state();
}

int get_player_socket(char role) {
    int socket;
    lock_game_state();
    if (role == 'X') {
        socket = game_state.player1_socket;
    } else if (role == 'O') {
        socket = game_state.player2_socket;
    } else {
        socket = -1;
    }
    unlock_game_state();
    return socket;
}