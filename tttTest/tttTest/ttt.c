#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define MAX_MESSAGE_LENGTH 1024

int connect_to_server(const char *ip, int port);

typedef struct {
    char* player_name[10];
    // Add any other game-specific state here
} game_state_t;

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 2 && argc != 1) {
        printf("Usage: %s [server_ip_address]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = (argc == 2) ? argv[1] : SERVER_IP;
    int client_socket = connect_to_server(ip, SERVER_PORT);
    printf("Enter PLAY <player_name> to start a match.\n");

    char input[250];
    char command[10];
    char msg[50];

    // Main game loop
    while (1) {
        // Get player name from command-line argument
        if ((fgets(input, sizeof(input), stdin) == NULL)) {
            perror("Error reading player move");
            break;
        }

        // Send player name to server
        if (sscanf(input, "%s %s", command, msg) == 2) {
            if(strcmp(command, "PLAY") == 0){
                write_msg(client_socket, msg);
                printf("%s", receive_msg(client_socket)); // Add this line to read the server's response
                printf("%s\n", format_message(MSG_WAIT));
                printf("%s", receive_msg(client_socket));
            } else{
                printf("Enter PLAY <player_name> to start a match.\n");
            }
            
            //send_message(client_socket, "C");
            if(strcmp(command, "MOVE") == 0){
                if (sscanf(msg, "%s %s", command, msg) == 2){
                    if(validate_move(msg) == 0){
                        perror("Error reading player move");
                        break;
                    } else {
                        write_msg(client_socket, input);
                    }
                }
                printf("%s\n", receive_msg(client_socket));
            }
            //send_message(client_socket, "C");
        } else {
            printf("Invalid command.\n");
        }
        
    }

    // Close connection to server
    close(client_socket);
    return 0;
}

int connect_to_server(const char *ip, int port) {
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set server address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_address.sin_addr) != 1) {
        perror("Error parsing server IP address");
        exit(EXIT_FAILURE);
    }

    printf("Attempting to connect to server at %s:%d\n", ip, port);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", ip, port);

    return sockfd;
}

int read_user_input(char *input) {
    // Read a line of input from the user
    // Return the number of characters read
    // Return -1 if there was an error
    if (fgets(input, MAX_MESSAGE_LENGTH, stdin) == NULL) {
        perror("Error reading user input");
        return -1;
    }

    return 0;
}
