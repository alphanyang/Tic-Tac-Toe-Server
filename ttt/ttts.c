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
#define BOARD_SIZE 9

typedef struct
{
	char name[MAX_NAME_LEN];
	char role;
	int sock_fd;
}

player_t;

typedef struct
{
	player_t players[2];
	int status;
	pthread_mutex_t lock;
	char board[BOARD_SIZE];
	int current_turn;
}
game_t;

game_t games[MAX_CLIENTS];
int num_games = 0;
pthread_mutex_t game_lock;
char player_names[MAX_CLIENTS *2][MAX_NAME_LEN];
int num_player_names = 0;
pthread_mutex_t player_name_lock;

void init_board(char board[BOARD_SIZE])
{
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			board[i] = '.';
		}
	}
}

int check_name(char *name)
{
	pthread_mutex_lock(&player_name_lock);
	for (int i = 0; i < num_player_names; i++)
	{
		if (strcmp(name, player_names[i]) == 0)
		{
			pthread_mutex_unlock(&player_name_lock);
			return 0;
		}
	}

	pthread_mutex_unlock(&player_name_lock);
	return 1;
}

void *handle_client(void *arg)
{
	int client_fd = *((int*) arg);
	free(arg);
	char *buf;
	char cmd[50];
	char msg[50];
	int game_id = -1;
	player_t player;

	// read player name
	buf = receive_msg(client_fd);
	if (buf == NULL)
	{
		close(client_fd);
		return NULL;
	}

	if (check_name(buf) == 0)
	{
		write_msg(client_fd, "INVL name already in use\n");
		close(client_fd);
		free(buf);
		return NULL;
	}
	else
	{
		pthread_mutex_lock(&player_name_lock);
		strcpy(player_names[num_player_names++], buf);
		pthread_mutex_unlock(&player_name_lock);
	}

	pthread_mutex_lock(&game_lock);
	for (int i = 0; i < num_games; i++)
	{
		if (games[i].status == 0 && strcmp(games[i].players[0].name, buf) != 0)
		{
			// join existing game
			game_id = i;
			//strcpy(player.name, buf);
			sscanf(buf, "%[^\n]", player.name);
			player.sock_fd = client_fd;
			player.role = 'O';
			games[i].players[1] = player;
			games[i].status = 1;
			pthread_mutex_unlock(&game_lock);
			sprintf(buf, "BEGN %c %s", games[i].players[1].role, games[i].players[0].name);
			write_msg(games[i].players[1].sock_fd, buf);
			sprintf(buf, "BEGN %c %s", games[i].players[0].role, games[i].players[1].name);
			write_msg(games[i].players[0].sock_fd, buf);
			break;
		}
	}

	if (game_id == -1)
	{
		// create new game
		game_id = num_games++;
		//strcpy(player.name, buf);
		sscanf(buf, "%[^\n]", player.name);
		player.sock_fd = client_fd;
		player.role = 'X';
		games[game_id].players[0] = player;
		games[game_id].status = 0;
		pthread_mutex_init(&games[game_id].lock, NULL);
		init_board(games[game_id].board);
		pthread_mutex_unlock(&game_lock);
		write_msg(client_fd, "WAIT");
		games[game_id].current_turn = 0;
	}

	// read player moves
	while (1)
	{
		buf = receive_msg(client_fd);
		printf("Received message: %s", buf);

		if (buf == NULL)
		{
			printf("Connection dropped by client %d\n", client_fd);
			close(client_fd);
			break;
		}

		// process player move
		pthread_mutex_lock(&games[game_id].lock);
		if (sscanf(buf, "%s %[^\n]", cmd, msg) == 2)
		{
			if (strcmp(cmd, "MOVE") == 0)
			{
				char pos[50];
				char role;
				if (sscanf(msg, "%c %s", &role, pos) == 2)
				{
					printf("Received move: %c %s\n", role, pos);
					if (validate_move(pos) == 0)
					{
						write_msg(client_fd, "!INVALID MOVE: Cell out of bounds");
					}
					else if(role!=player.role){
						write_msg(client_fd, "!INVALID MOVE: Not your role");
					}
					else
					{
					 	// Check if it's the current player's turn
						int player_index = (player.role == games[game_id].players[0].role) ? 0 : 1;
						if (games[game_id].current_turn != player_index)
						{
							write_msg(client_fd, "INV Not your turn");
						}
						else
						{
						 	// TODO: process and update the game state based on the move
							// send updated game state back to both clients
							int row = pos[0] - '1';
							int col = pos[2] - '1';
							int index = row *3 + col;
							if (games[game_id].board[index] == '.')
							{
								games[game_id].board[index] = player.role;

								// Check for win condition
								if (check_win(games[game_id].board))
								{
								 	// Announce winner
                                    if(strcmp(player.name, games[game_id].players[0].name) == 0)
                                    {
                                        sprintf(buf, "OVER L %s won\n%s\n", games[game_id].players[0].name, games[game_id].board);
                                        write_msg(games[game_id].players[1].sock_fd, buf);
                                        sprintf(buf, "OVER W %s won\n%s\n", games[game_id].players[0].name,  games[game_id].board);
                                        write_msg(games[game_id].players[0].sock_fd, buf);
                                        break;
                                    }
                                    else if(strcmp(player.name, games[game_id].players[1].name) == 0)
                                    {
										//send final board
										// sprintf(buf, "MOVD %s %s %s\n", role, pos, games[game_id].board);
										// write_msg(games[game_id].players[0].sock_fd, buf);
										// write_msg(games[game_id].players[1].sock_fd, buf);
                                        sprintf(buf, "OVER W %s won\n%s\n", games[game_id].players[1].name,  games[game_id].board);
                                        write_msg(games[game_id].players[1].sock_fd, buf);
                                        sprintf(buf, "OVER W %s won\n%s\n", games[game_id].players[1].name,  games[game_id].board);
                                        write_msg(games[game_id].players[0].sock_fd, buf);
                                        break;
                                    }
								}
								else if (check_draw(games[game_id].board))
								{
								 	// Announce draw
									sprintf(buf, "OVER D Game has ended in a draw.\n");
									write_msg(games[game_id].players[0].sock_fd, buf);
									write_msg(games[game_id].players[1].sock_fd, buf);
									break;
								}

								if (games[game_id].current_turn == player_index)
								{
									sprintf(buf, "MOVD %c %s %s\n", role, pos, games[game_id].board);
									write_msg(games[game_id].players[0].sock_fd, buf);
									write_msg(games[game_id].players[1].sock_fd, buf);
								}

								// Update current turn
								games[game_id].current_turn = 1 - games[game_id].current_turn;
							}
							else
							{
							 					// Invalid move (cell already occupied) - inform player
								write_msg(client_fd, "!INVALID MOVE: Cell already occupied");
							}

							// Send updated game state to both players

						}
					}
				}
			}
            // else if (strcmp(cmd, "RSGN") == 0)
            // {
			// 	// Inform other player that the current player resigned
            //     close(client_fd);
            //     return NULL;
            // }
            else if (strcmp(cmd, "DRAW") == 0)
            {
                // Send other client draw request or process the draw response
                int player_index = (player.role == games[game_id].players[0].role) ? 0 : 1;
                int other_player_index = 1 - player_index;
                if (strcmp(msg, "S") == 0)
                {
                    // Send draw request to the other player
                    write_msg(games[game_id].players[other_player_index].sock_fd, "DRAW S");
                }
                else if (strcmp(msg, "A") == 0)
                {
                    // The current player accepted the draw request, inform both players
                    sprintf(buf, "OVER D Game has ended in a draw.\n");
                    write_msg(games[game_id].players[0].sock_fd, buf);
                    write_msg(games[game_id].players[1].sock_fd, buf);
                }
                else if (strcmp(msg, "R") == 0)
                {
                    // The current player declined the draw request, inform the other player
                    write_msg(games[game_id].players[other_player_index].sock_fd, "DRAW R");
                }
            }
		}
        else if(sscanf(buf, "%s %[^\n]", cmd, msg) == 1){
            if (strcmp(cmd, "RSGN") == 0)
            {
                
                if(strcmp(player.name, games[game_id].players[1].name) == 0)
                {
                    sprintf(buf, "OVER L %s won %s resigned\n", games[game_id].players[0].name, games[game_id].players[1].name);
                    write_msg(games[game_id].players[0].sock_fd, buf);
                    sprintf(buf, "OVER W %s won %s resigned\n", games[game_id].players[0].name, games[game_id].players[1].name);
                    write_msg(games[game_id].players[1].sock_fd, buf);
                    break;
                }
                else if(strcmp(player.name, games[game_id].players[0].name) == 0)
                {
                    sprintf(buf, "OVER L %s won %s resigned\n", games[game_id].players[1].name, games[game_id].players[0].name);
                    write_msg(games[game_id].players[0].sock_fd, buf);
                    sprintf(buf, "OVER W %s won %s resigned\n", games[game_id].players[1].name, games[game_id].players[0].name);
                    write_msg(games[game_id].players[1].sock_fd, buf);
                    break;
                }
                close(client_fd);
                return NULL;
            }
        }
		else
		{
			printf("Invalid command.\n");
		}

		pthread_mutex_unlock(&games[game_id].lock);
	}

	// player disconnected
	if (game_id != -1)
	{
		// inform the other player that the game has ended
		char buf[2500];	// increase buffer size
		if (client_fd == games[game_id].players[0].sock_fd && games[game_id].players[1].sock_fd != -1)
		{
			snprintf(buf, sizeof(buf), "Player %s disconnected.\n", games[game_id].players[0].name);
			write_msg(games[game_id].players[1].sock_fd, buf);
		}
		else if (client_fd == games[game_id].players[1].sock_fd && games[game_id].players[0].sock_fd != -1)
		{
			snprintf(buf, sizeof(buf), "Player %s disconnected.\n", games[game_id].players[1].name);
			write_msg(games[game_id].players[0].sock_fd, buf);
		}

		if (client_fd == games[game_id].players[0].sock_fd)
		{
			if (games[game_id].status == 0)
			{
			 	// player 0 disconnected before game started
				pthread_mutex_lock(&game_lock);
				for (int i = 0; i < num_games; i++)
				{
					if (i == game_id)
					{
						continue;
					}

					if (games[i].status == 0 && games[i].players[0].sock_fd == -1)
					{
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

				if (game_id == num_games - 1)
				{
				 		// remove empty game from list
					num_games--;
				}
				else
				{
				 		// move last game in list to empty spot
					games[game_id] = games[num_games - 1];
					num_games--;
				}

				pthread_mutex_unlock(&game_lock);
			}
			else
			{
			 	// player 0 disconnected during game
				games[game_id].players[0].sock_fd = -1;
				pthread_mutex_unlock(&games[game_id].lock);
			}
		}
		else if (client_fd == games[game_id].players[1].sock_fd)
		{
			// player 1 disconnected
			if (games[game_id].status == 0)
			{
			 	// player 1 disconnected before game started
				pthread_mutex_lock(&game_lock);
				for (int i = 0; i < num_games; i++)
				{
					if (i == game_id)
					{
						continue;
					}

					if (games[i].status == 0 && games[i].players[0].sock_fd == -1)
					{
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

				if (game_id == num_games - 1)
				{
				 	// remove empty game from list
					num_games--;
				}
				else{
				 	//move last game in list to empty spot
					games[game_id] = games[num_games - 1];
					num_games--;
				}

				pthread_mutex_unlock(&game_lock);
			}
			else{
			 	// player 1 disconnected during game
				games[game_id].players[1].sock_fd = -1;
				pthread_mutex_unlock(&games[game_id].lock);
			}
		}

		close(client_fd);
	}

	// cleanup thread resources
	free(buf);
	pthread_join(pthread_self(), NULL);

	return NULL;
}

int main(){
	int server_fd, client_fd;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	pthread_t client_thread;

	// create server socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0){
		perror("socket");
		exit(1);
	}

	// set SO_REUSEADDR option
	int reuseaddr = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0){
		perror("setsockopt");
		exit(1);
	}

	// set server address
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// bind server socket to address
	if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0){
		perror("bind");
		exit(1);
	}

	// start listening for connections
	if (listen(server_fd, MAX_CLIENTS) < 0){
		perror("listen");
		exit(1);
	}

	while (1)
	{
		// accept client connection
		client_fd = accept(server_fd, (struct sockaddr *) &address, (socklen_t*) &addrlen);
		if (client_fd < 0) {
			perror("accept");
			exit(1);
		}

		int *client_fd_ptr = malloc(sizeof(int));
		*client_fd_ptr = client_fd;
		// create thread to handle client
		if (pthread_create(&client_thread, NULL, handle_client, client_fd_ptr) != 0) {
			perror("pthread_create");
			exit(1);
		}

		if (pthread_mutex_init(&player_name_lock, NULL) != 0) {
			perror("pthread_mutex_init");
			exit(1);
		}

		pthread_detach(client_thread);
	}

	close(server_fd);
	return 0;
}
