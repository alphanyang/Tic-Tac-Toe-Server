# Contributors
-Alphan Yang ay329
-Lily Xing

# Tic-Tac-Toe Server

This is a simple Tic-Tac-Toe server application that allows clients to connect and play the game with each other.

## Usage

The program can be run with the following command:

```bash
./server
```

## Features

- Supports multiple concurrent games
- Thread-safe handling of games and player names
- Text-based protocol for communication between server and clients

## Protocol

The protocol consists of simple text commands and responses, as shown below:

- `MOVE <role> <position>`: Send a move to the server, where `<role>` is either 'X' or 'O' and `<position>` is the row and column of the move (e.g., "1 2").
- `RSGN`: Resign from the current game.
- `DRAW <response>`: Send a draw request to the other player, where `<response>` can be 'S' for sending a request, 'A' for accepting, and 'R' for rejecting.

## Server Responses

- `BEGN <role> <opponent_name>`: Begin a new game, where `<role>` is either 'X' or 'O' and `<opponent_name>` is the name of the other player.
- `WAIT`: Wait for another player to join the game.
- `INV`: Invalid command or move.
- `MOVD <role> <position> <board>`: The move has been made, where `<role>` is either 'X' or 'O', `<position>` is the row and column of the move, and `<board>` is the current game board.
- `OVER <result> <message>`: The game is over, where `<result>` can be 'W' for win, 'L' for lose, or 'D' for draw, and `<message>` contains additional information about the game result.

## Code Structure

The code is structured as follows:

- `player_t` struct: Represents a player, containing their name, role ('X' or 'O'), and socket file descriptor.
- `game_t` struct: Represents a game, containing two players, status, lock for synchronization, board, and current turn.
- `init_board()`: Initialize the game board.
- `check_name()`: Check if a player name is already in use.
- `handle_client()`: Handle communication with a connected client.
- `main()`: Start the server and accept incoming connections.

# Client

This program implements a client for the TIC-TAC-TOE game protocol. It connects to a server at a given IP address and port, and then reads user input and sends messages to the server. The client also receives messages from the server and prints them to the console.

## Usage

The program can be run with the following command:

```bash
./client <server_ip_address>
```
If server_ip_address is not provided, it will default to 127.0.0.1.

Once the client is connected to the server, it will read user input from the console and send messages to the server based on the input. It will also receive messages from the server and print them to the console.

## Functionality
The client can handle the following commands from the user:

PLAY <message>: Sends a message to the server with the given message.
MOVE <role> <pos>: Sends a move to the server, where role is either X or O, and pos is the position to place the move in.
DRAW A: Accepts a draw request from the opponent.
DRAW R: Rejects a draw request from the opponent.
RSGN: Resigns from the game.
The client can handle the following messages from the server:

WAIT: Informs the client that it is waiting for another player to join the game.
BEGN <role> <opponent_name>: Informs the client that the game has started, and provides the client's role and the name of the opponent.
MOVD <role> <pos> <grid>: Informs the client that a move has been made by the given role at the given position, and provides the current state of the grid.
INVL <reason>: Informs the client that a move or message was invalid, and provides a reason for the invalidity.
DRAW S: Informs the client that the opponent has suggested a draw.
DRAW R: Informs the client that the opponent has rejected a draw request.
OVER <result>: Informs the client that the game is over, and provides the result of the game.

# Protocol

This repository contains a C code file protocol.c that implements the game protocol for a tic-tac-toe game. The code defines functions for sending and receiving messages between a server and clients, validating moves, and checking for win/draw conditions.

## Functions

### write_msg
```void write_msg(int sock_fd, char *msg)```
This function sends a message through a socket. It takes in a socket file descriptor sock_fd and a message string msg, and sends the message through the socket. If an error occurs while sending the message, the function prints an error message to stderr.

### receive_msg
```char *receive_msg(int sock_fd)```
This function receives a message from a socket. It takes in a socket file descriptor sock_fd, receives a message from the socket, and returns the message as a string. If an error occurs while receiving the message, the function returns NULL.

### parse_index
```int parse_index(char *move)```
This function parses a move string and returns the corresponding board index. It takes in a move string in the format "row,col" and returns the board index as an integer.

### validate_move
```int validate_move(const char *move)```
This function validates a move string. It takes in a move string in the format "row,col", and checks if it has the correct format and is within the valid range. If the move is valid, the function returns 1. Otherwise, it returns 0.

### check_win
This function checks if a player has won the game. It takes in the current state of the grid as a string board and checks if any rows, columns, or diagonals have the same symbol (X or O). If a win condition is met, the function returns 1. Otherwise, it returns 0.

### check_draw
This function checks if the game has ended in a draw. It takes in the current state of the grid as a string board and

