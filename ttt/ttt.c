#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define MAX_MESSAGE_LENGTH 1024

int connect_to_server(const char *ip, int port);

void handle_user_input(int server_fd)
{
    char input[250];
    char command[10];
    char msg[50];

    if ((fgets(input, sizeof(input), stdin) == NULL))
    {
        perror("Error reading player move");
        return;
    }

    // Sending input
    if (sscanf(input, "%s %[^\n]", command, msg) == 2)
    {
        if (strcmp(command, "PLAY") == 0)
        {
            write_msg(server_fd, msg);
        }
        else if (strcmp(command, "MOVE") == 0)
        {
            write_msg(server_fd, input);
        }
        else if (strcmp(command, "DRAW") == 0)
        {
            write_msg(server_fd, input);
            //wait for response;
        }
        else
        {
            printf("Invalid command.\n");
        }
    }
    else if (sscanf(input, "%s %[^\n]", command, msg) == 1)
    {
        if (strcmp(command, "RSGN") == 0)
        {
            write_msg(server_fd, input);
        }
		else printf("Invalid command.\n");
    }
    else{
        printf("Invalid command.\n");
    }
}

void handle_server_messages(int server_fd)
{

	while (1)
	{
		// Initialize file descriptor set
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(server_fd, &read_fds);
		FD_SET(STDIN_FILENO, &read_fds);

		int max_fd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;

		// Wait for data to be available on either the server or stdin
		int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		if (activity == -1)
		{
			perror("Error with select");
			break;
		}

		// If data is available on the server
		if (FD_ISSET(server_fd, &read_fds))
		{
			char *response = receive_msg(server_fd);
			if (response == NULL)
			{
				perror("Error receiving message from server");
				exit(EXIT_FAILURE);
			}

			// Process the received message
			char cmd[50];
            char role;
            char grid[50];
            char opponent_name[MAX_MESSAGE_LENGTH];
            char server_response[50];
			if (sscanf(response, "%s %[^\n]", cmd, server_response) == 2)
			{
				if (strcmp(cmd, "WAIT") == 0)
				{
					printf("WAIT\n");
				}
				else if (strcmp(cmd, "BEGN") == 0)
                {
                    printf("%s\n", response);
                    
                    
                    if (sscanf(response, "BEGN %c %[^\n]", &role, opponent_name) == 2)
                    {
                        printf("Game started. You are %c. Playing against %s.\n", role, opponent_name);
                        if (role == 'X')
                        {
                            printf("Your turn. Enter MOVE <role> <pos> to make a move.\n");
                        }
                        else
                        {
                            printf("Waiting for %s to make a move.\n", opponent_name);
                        }
                    }
                }

				else if (strcmp(cmd, "MOVD") == 0)
				{
					char moved_role;
					if (sscanf(response, "MOVD %c %s %[^\n]", &moved_role, cmd, grid) == 3)
					{
                        printf("%s", response);
						for(int r = 0; r < 3; r++){
							for(int c = 0; c < 3; c++){
								printf("%c ", grid[r*3 + c]);
							}
							printf("\n");
						}
                        if (role == moved_role) // Compare characters directly, without using '&'
                        {
                            printf("Waiting for %s to make a move.\n", opponent_name);
                        }
                        else
                        {
                            printf("Your turn. Enter MOVE <role> <pos> to make a move.\n");
                        }
					}
				}
				else if (strcmp(cmd, "INVL") == 0)
				{
					char reason[MAX_MESSAGE_LENGTH];
					if (sscanf(response, "INVL %[^\n]", reason) == 1)
					{
						printf("Invalid move or message: %s\n", reason);
                        // Call the function to get another input from the user
					}
				}
				else if (strcmp(cmd, "DRAW") == 0)
				{
					char subcmd;
					if (sscanf(response, "DRAW %c", &subcmd) == 1)
					{
						if (subcmd == 'S')
						{
							printf("Opponent suggests a draw. Enter DRAW A to accept or DRAW R to reject.\n");
						}
						else if (subcmd == 'R')
						{
							printf("Opponent rejected the draw.\n");
						}
					}
				}
				else if (strcmp(cmd, "OVER") == 0)
				{
					printf("OVER %s\n", server_response);
					//check if server response contains resigned else dont print grid
					char *resigned = strrchr(server_response, ' ');
					if(resigned && !strcmp(resigned, "resigned")){
						if(sscanf(response, "OVER %[^\n]%s", cmd, grid) == 2){
							for(int r = 0; r < 3; r++){
								for(int c = 0; c < 3; c++){
									printf("%c ", grid[r*3 + c]);
								}
								printf("\n");
							}
						}
					}
					break;
				}
				else
				{
					printf("%s\n", response);
				}
			}
			else
			{
				printf("%s\n", response);
			}

			if (strstr(response, "disconnected") != NULL)
			{
				printf("OVER W You win.\n");
				close(server_fd);
				exit(EXIT_SUCCESS);
			}

			free(response);
		}

		// If data is available on stdin
		if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            handle_user_input(server_fd);
        }
	}
}

int main(int argc, char *argv[])
{
	// Check command-line arguments
	if (argc != 2 && argc != 1)
	{
		printf("Usage: %s[server_ip_address]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *ip = (argc == 2) ? argv[1] : SERVER_IP;
	int client_socket = connect_to_server(ip, SERVER_PORT);

	handle_server_messages(client_socket);

	// Close connection to server
	close(client_socket);
	return 0;
}

int connect_to_server(const char *ip, int port)
{
	// Create a socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

	// Set server address
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &server_address.sin_addr) != 1)
	{
		perror("Error parsing server IP address");
		exit(EXIT_FAILURE);
	}

	printf("Attempting to connect to server at %s:%d\n", ip, port);

	// Connect to server
	if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
	{
		perror("Error connecting to server");
		exit(EXIT_FAILURE);
	}

	printf("Connected to server at %s:%d\n", ip, port);

	return sockfd;
}

int read_user_input(char *input)
{
	// Read a line of input from the user
	// Return the number of characters read
	// Return -1 if there was an error
	if (fgets(input, MAX_MESSAGE_LENGTH, stdin) == NULL)
	{
		perror("Error reading user input");
		return -1;
	}

	return 0;
}
