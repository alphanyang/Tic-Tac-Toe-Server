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
    player_t players[2];
    int status;
    pthread_mutex_t lock;
} game_t;

game_t games[MAX_CLIENTS];
int num_games = 0;
pthread_mutex_t game_lock;

void send_msg(int sock_fd, char *msg) {
    int len = strlen(msg);
    send(sock_fd, msg, len, 0);
}

char *recieve_msg(int sock_fd) {
    char *buf = (char *)malloc(256 * sizeof(char));
    int bytes_read = recv(sock_fd, buf, 256, 0);
    if (bytes_read <= 0) {
        free(buf);
        return NULL;
    }
    buf[bytes_read] = '\0';
    return buf;
}

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

void *handle_client(void *arg) {
    int client_fd = *(int*)arg;
    char *buf;
    int game_id = -1;
    player_t player;

    // read player name
    buf = recieve_msg(client_fd);
    if (buf == NULL) {
        close(client_fd);
        return NULL;
    }
    if (check_name(buf) == 0) {
        send_msg(client_fd, "INVL name already in use\n");
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
            send_msg(games[i].players[1].sock_fd, buf);
            sprintf(buf, "START %s\n", games[i].players[1].name);
            send_msg(games[i].players[0].sock_fd, buf);
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
        pthread_mutex_init(&games[game_id].lock, NULL);
        pthread_mutex_unlock(&game_lock);
    }
    free(buf);

    // read player moves
    while (1) {
        buf = recieve_msg(client_fd);
        if (buf == NULL) {
            // player disconnected
                break;
        }
        // process player move
        pthread_mutex_lock(&games[game_id].lock);
        // TODO: process player move
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
                        send_msg(games[i].players[1].sock_fd, buf);
                        sprintf(buf, "START %s\n", games[i].players[1].name);
                        send_msg(games[i].players[0].sock_fd, buf);
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
                        send_msg(games[i].players[1].sock_fd, buf);
                        sprintf(buf, "START %s\n", games[i].players[1].name);
                        send_msg(games[i].players[0].sock_fd, buf);
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

