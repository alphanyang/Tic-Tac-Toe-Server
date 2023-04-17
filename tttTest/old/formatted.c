#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define PORT 5000
#define MAX_CLIENTS 10
#define MAX_NAME_LEN 20

typedef struct {
    char name[MAX_NAME_LEN];
    int sock_fd;
} player_t;

typedef struct {
    char board[10];
    player_t players[2];
    int status;
    pthread_mutex_t lock;
} game_t;

game_t games[MAX_CLIENTS];
int num_games = 0;
pthread_mutex_t game_lock;

int check_name(char *name) {
    for (int i = 0; i < num_games; i++) {
        if (games[i].status != 0) {
            if (strcmp(name, games[i].players[0].name) == 0 || strcmp(name, games[i].players[1].name) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

void init_board(game_t *game) {
    memset(game->board, ' ', 9);
}

int process_move(game_t *game, char *move) {
    int pos = atoi(move);
    if (pos < 1 || pos > 9) {
    return 0;
    }
    pos--;
    if (game->board[pos] != ' ') {
    return 0;
    }
    return 1;
}

void update_game_state(game_t *game, char *move) {
    int pos = atoi(move);
    pos--;
    if (game->players[0].sock_fd == game->players[1].sock_fd) {
        // single player game
        game->board[pos] = 'X';
    } else {
        // two player game
        if (game->board[pos] == 'X') {
            game->board[pos] = 'O';
        } else {
            game->board[pos] = 'X';
        }
    }
}

char gamer_over(game_t *game) {
    int i;
    // check rows
    for (i = 0; i < 9; i += 3) {
    if (game->board[i] != ' ' && game->board[i] == game->board[i+1] && game->board[i+1] == game->board[i+2]) {
    return game->board[i];
    }
    }
    // check columns
    for (i = 0; i < 3; i++) {
    if (game->board[i] != ' ' && game->board[i] == game->board[i+3] && game->board[i+3] == game->board[i+6]) {
    return game->board[i];
    }
    }
    // check diagonals
    if (game->board[0] != ' ' && game->board[0] == game->board[4] && game->board[4] == game->board[8]) {
    return game->board[0];
    }
    if (game->board[2]!= ' ' && game->board[2] == game->board[4] && game->board[4] == game->board[6]) {
    return game->board[2];
    }
    // check for tie
    for (i = 0; i < 9; i++) {
    if (game->board[i] == ' ') {
    return 'C';
    }
    }
    return 'T';
}

void send_MOVD_to_both_players(game_t *game) {
    char buf[32];
    sprintf(buf, "MOVD %s\n", game->board);
    write_msg(game->players[0].sock_fd, buf);
    write_msg(game->players[1].sock_fd, buf);
}

void send_OVER_to_both_players(game_t *game, char *msg) {
    char buf[32];
    if (game->players[0].sock_fd == game->players[1].sock_fd) {
        // single player game
        sprintf(buf, "OVER %s\n", msg);
        write_msg(game->players[0].sock_fd, buf);
    } else {
    // two player game
        sprintf(buf, "OVER %s X\n", msg);
        write_msg(game->players[0].sock_fd, buf);
        sprintf(buf, "OVER %s O\n", msg);
        write_msg(game->players[1].sock_fd, buf);
    }
}

void send_DRAW_to_opponent(int client_fd, game_t *game) {
    if (client_fd == game->players[0].sock_fd) {
        write_msg(game->players[1].sock_fd, "DRAW A\n");    
    } else {
        write_msg(game->players[0].sock_fd, "DRAW A\n");    
    }
}

void send_DRAW_R_to_client(int client_fd, game_t *game) {
    if (client_fd == game->players[0].sock_fd) {
        write_msg(client_fd, "DRAW R\n");
    } else {
        write_msg(client_fd, "DRAW R\n");
}
}

void send_INVL_to_client(int client_fd, char *msg) {
    char buf[32];
    sprintf(buf, "INVL %s\n", msg);
    write_msg(client_fd, buf);
}

void *handle_client(void *arg) {
    int client_fd = *(int*)arg;
    char *buf;
    int game_id = -1;
    player_t player;

    // read player name
    buf = read_msg(client_fd);
    if (buf == NULL) {
        close(client_fd);
        return NULL;
    }
    if (check_name(buf) == 0) {
        write_msg(client_fd, "INVL name already in use\n");
        free(buf);
        close(client_fd);
        return NULL;
    }
    pthread_mutex_lock(&game_lock);
    for (int i = 0; i < num_games; i++) {
        if (games[i].status == 0 && strcmp(games[i].players[0].name, buf) != 0) {
            // join existing game
            game_id = i;
            strcpy(player.name, buf);
            player.sock_fd = client_fd;
            games[i].players[1] = player;
            games[i].status = 1;
            pthread_mutex_unlock(&game_lock);
            sprintf(buf, "START %s\n", games[i].players[0].name);
            write_msg(games[i].players[1].sock_fd, buf);
            sprintf(buf, "START %s\n", games[i].players[1].name);
            write_msg(games[i].players[0].sock_fd, buf);
            break;
        }
    }
    if (game_id == -1) {
        // create new game
        game_id = num_games++;
        strcpy(player.name, buf);
        player.sock_fd = client_fd;
        games[game_id].players[0] = player;
        games[game_id].status = 0;
        init_board(&games[game_id]);
        pthread_mutex_init(&games[game_id].lock, NULL);
        pthread_mutex_unlock(&game_lock);
    }
    free(buf);

    // read player moves
    // read player moves
    while (gamer_over(&games[game_id]) == 'C') {
        buf = read_msg(client_fd);
        if (buf == NULL) {
            // player disconnected
            break;
        }
        // process player move
        pthread_mutex_lock(&games[game_id].lock);

        char *board_copy = malloc(sizeof(game_t));
        *memcpy(board_copy, &games[game_id], sizeof(game_t));
        char *message_type = strtok(buf, " ");
        if (strcmp(message_type, "MOVE") == 0) {
        char *move = strtok(NULL, " ");
        int move_validity = process_move(board_copy, move);
        if (move_validity == 1) {
            // move was valid, update game state
            update_game_state(&games[game_id], move);
            send_MOVD_to_both_players(&games[game_id]);
        } else {
            // move was invalid, handle accordingly
            send_INVL_to_client(client_fd, "Invalid move");
        }
        } else if (strcmp(message_type, "RSGN") == 0) {
            send_OVER_to_both_players(&games[game_id], "Resigned");
        } else if (strcmp(message_type, "DRAW") == 0) {
            char *response = strtok(NULL, " ");
            if (strcmp(response, "S") == 0) {
                send_DRAW_to_opponent(client_fd, &games[game_id]);
            } else if (strcmp(response, "A") == 0) {
                send_OVER_to_both_players(&games[game_id], "Draw");
            } else if (strcmp(response, "R") == 0) {
                send_DRAW_R_to_client(client_fd, &games[game_id]);
            } else {
                send_INVL_to_client(client_fd, "!Invalid DRAW message");
            }
        } else {
            send_INVL_to_client(client_fd, "!Invalid message type");
        }

        free(board_copy);
        pthread_mutex_unlock(&games[game_id].lock);
        free(buf);
    }

    // player disconnected
    if (game_id != -1) {
        if (client_fd == games[game_id].players[0].sock_fd) {
            if (games[game_id].status == 0) {
                // player 0 disconnected before game started
                pthread_mutex_lock(&game_lock);
                for (int i = 0; i < num_games; i++) {
                    if (i == game_id) {
                        continue;
                    }
                    if (games[i].status == 0 && games[i].players[0].sock_fd == -1) {
                        // take over empty slot in other game
                        games[i].players[0] = games[game_id].players[1];
                        games[i].status = 1;
                        pthread_mutex_unlock(&game_lock);
                        sprintf(buf, "START %s\n", games[i].players[0].name);
                        write_msg(games[i].players[1].sock_fd, buf);
                        sprintf(buf, "START %s\n", games[i].players[1].name);
                        write_msg(games[i].players[0].sock_fd, buf);
                        break;
                    }
                }
                if (game_id == num_games-1) {
                    // remove empty game from list
                    num_games--;
                } else {
                    // move last game in list to empty spot
                    games[game_id] = games[num_games-1];
                    num_games--;
                }
                pthread_mutex_unlock(&game_lock);
            } else {
                // player 0 disconnected during game
                games[game_id].players[0].sock_fd = -1;
                pthread_mutex_unlock(&games[game_id].lock);
            }
        } else if (client_fd == games[game_id].players[1].sock_fd) {
            // player 1 disconnected
            if (games[game_id].status == 0) {
                // player 1 disconnected before game started
                pthread_mutex_lock(&game_lock);
                for (int i = 0; i < num_games; i++) {
                    if (i == game_id) {
                        continue;
                    }
                    if (games[i].status == 0 && games[i].players[0].sock_fd == -1) {
                        // take over empty slot in other game
                        games[i].players[0] = games[game_id].players[0];
                        games[i].status = 1;
                        pthread_mutex_unlock(&game_lock);
                        sprintf(buf, "START %s\n", games[i].players[0].name);
                        write_msg(games[i].players[1].sock_fd, buf);
                        sprintf(buf, "START %s\n", games[i].players[1].name);
                        write_msg(games[i].players[0].sock_fd, buf);
                        break;
                    }
                }
                if (game_id == num_games-1) {
                    // remove empty game from list
                    num_games--;
                } else {
                    // move last game in list to empty spot
                    games[game_id] = games[num_games-1];
                    num_games--;
                }
                pthread_mutex_unlock(&game_lock);
            } else {
                // player 1 disconnected during game
                games[game_id].players[1].sock_fd = -1;
                pthread_mutex_unlock(&games[game_id].lock);
            }
        }
       close(client_fd);
    }
    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t client_thread;

    // create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    // set server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // bind server socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(1);
    }

    // start listening for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(1);
    }

    while (1) {
        // accept client connection
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            perror("accept");
            exit(1);
        }

        // create thread to handle client
        if (pthread_create(&client_thread, NULL, handle_client, &client_fd) != 0) {
            perror("pthread_create");
            exit(1);
        }
        pthread_detach(client_thread);
    }

    close(server_fd);
    return 0;
}

