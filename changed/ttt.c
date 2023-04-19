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
    char input[256];
    char command[5];
    char msg[250];
	unsigned long length;

    if ((fgets(input, sizeof(input), stdin) == NULL))
    {
        perror("Error reading player move");
        return;
    }

    // Sending input
	if(input[strlen(input)-2]!='|') printf("INVL|15|Invalid format|\n");
    else if (sscanf(input, "%4s|%ld|%[^\n]", command, &length, msg) == 3)
    {
        if (strcmp(command, "PLAY") == 0)
        {	
			char name[250];
			if (sscanf(msg, "%[^|]|", name) == 1){
				if(strlen(name)+1==length) write_msg(server_fd, name);
				else printf("INVL|22|Length does not match|\n");
			}
			else printf("INVL|16|Improper format|\n");
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
            printf("INVL|16|Invalid command|\n");
        }
    }
	else if (sscanf(input, "%4s|%ld", command, &length) == 2){
		if (strcmp(command, "RSGN") == 0 && length==0) write_msg(server_fd, input);
		else printf("INVL|22|Length does not match|\n");
	}
    else{
        printf("INVL|16|Invalid command|\n");
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
			else printf("%s\n", response);
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