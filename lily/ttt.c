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
#define SERVER_PORT 5037
#define MAX_MESSAGE_LENGTH 1024

int connect_to_server(const char *ip, int port);
void send_message(int sockfd, const char *msg);
char *receive_message(int sockfd);
int read_user_input(char *input);

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

    // Wait for game to start
    // game_state_t game_state;
    // if (recv(client_socket, &game_state, sizeof(game_state_t), 0) == -1) {
    //     perror("Error receiving game state from server");
    //     exit(EXIT_FAILURE);
    // }

    // Main game loop
    while (1) {
        char move[256];
        if(fgets(move, 256, stdin) == NULL) {
            perror("Error reading message");
            break;
        }
        if (strncasecmp(move, "PLAY", 4)==0) {
            command[5];
            player_name[256];
            sscanf(move, "%s %s", command, player_name);
            if (send(client_socket, command, strlen(player_name) + 1, 0) == -1) {
                perror("Error sending player name to server");
                break;
            }
            // Receive waiting message from server
            char *waiting_msg = receive_message(client_socket);
            printf("%s ", waiting_msg);
        } else if(send(client_socket, move, strlen(move)+1, 0)==-1){
            perror("error sending move to server");
            break;
        }
        //recieve message
        //make sure if message is draw that client is responding properly
    }
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

    // Connect to server
    if (connect(sockfd, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

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