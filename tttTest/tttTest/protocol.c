#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

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

void write_msg(int sock_fd, char *msg) {
    int len = strlen(msg);
    if (write(sock_fd, msg, len) < 0) {
        fprintf(stderr, "Error sending message\n");
        return;
    }
}

char *receive_msg(int sock_fd) {
    char *buf = (char *)malloc(BUFFER_SIZE * sizeof(char));
    int bytes_read = read(sock_fd, buf, BUFFER_SIZE);
    if (bytes_read <= 0) {
        free(buf);
        return NULL;
    }
    buf[bytes_read] = '\0';
    return buf;
}

// char* read_msg(int sockfd) {
//     static char buffer[BUFFER_SIZE];
//     int nbytes = 0;
//     fd_set rfds;
//     struct timeval tv;

//     // Set timeout to 5 seconds
//     tv.tv_sec = 5;

//     // Clear the set of file descriptors
//     FD_ZERO(&rfds);
//     // Add the socket file descriptor to the set
//     FD_SET(sockfd, &rfds);

//     // Wait for data to be ready to read or timeout
//     int retval = select(sockfd+1, &rfds, NULL, NULL, &tv);
//     if (retval == -1) {
//         perror("Error in select()");
//         exit(EXIT_FAILURE);
//     } else if (retval == 0) {
//         // Timeout occurred
//         return NULL;
//     }

//     // Read data from the socket
//     nbytes = read(sockfd, buffer, BUFFER_SIZE-1);
//     if (nbytes == -1) {
//         perror("Error receiving message");
//         exit(EXIT_FAILURE);
//     } else if (nbytes == 0) {
//         // Connection closed by remote host
//         return NULL;
//     }

//     // Null-terminate the buffer
//     buffer[nbytes] = '\0';

//     return buffer;
// }

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
    char content[BUFFER_SIZE] = {0};
    va_list args;
    va_start(args, msg_type);

    if (strcmp(msg_type, "NAME") == 0) {
        snprintf(content, BUFFER_SIZE, "%s|", va_arg(args, char *));
    } else if (strcmp(msg_type, "MOVE") == 0) {
        snprintf(content, BUFFER_SIZE, "%s|%s|", va_arg(args, char *), va_arg(args, char *));
    } else if (strcmp(msg_type, "BEGN") == 0) {
        snprintf(content, BUFFER_SIZE, "%s|%s|", va_arg(args, char *), va_arg(args, char *));
    } else if (strcmp(msg_type, "MOVD") == 0) {
        snprintf(content, BUFFER_SIZE, "%s|%s|%s|", va_arg(args, char *), va_arg(args, char *), va_arg(args, char *));
    } else if (strcmp(msg_type, "INVL") == 0) {
        snprintf(content, BUFFER_SIZE, "%s|", va_arg(args, char *));
    } else if (strcmp(msg_type, "RSGN") == 0 || strcmp(msg_type, "WAIT") == 0) {
        // Do nothing since these messages don't have any content.
    } else if (strcmp(msg_type, "DRAW") == 0) {
        snprintf(content, BUFFER_SIZE, "%s|", va_arg(args, char *));
    } else if (strcmp(msg_type, "OVER") == 0) {
        snprintf(content, BUFFER_SIZE, "%s|%s|", va_arg(args, char *), va_arg(args, char *));
    }

    va_end(args);

    if (strcmp(msg_type, "WAIT") == 0) {
        formatted_msg = (char *)malloc(strlen("WAIT|0|") + 1);
        snprintf(formatted_msg, strlen("WAIT|0|") + 1, "WAIT|0|");
    } else {
        int size = strlen(content);
        if (strcmp(msg_type, "NAME") == 0) {
            size++;  // Add one to the size for the vertical bar in the "NAME" case.
        }
        formatted_msg = (char *)malloc(strlen(msg_type) + 1 + 3 + 1 + size + 1);
        snprintf(formatted_msg, strlen(msg_type) + 1 + 1 + 3 + size + 1, "%s|%d|%s\n", msg_type, size, content); // Corrected buffer size
    }

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
    return 'C';
}
