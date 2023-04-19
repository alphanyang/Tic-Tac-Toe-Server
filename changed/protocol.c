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

#define BUFFER_SIZE 512
#define BOARD_SIZE 9

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

int parse_index(char *move){
    int row = move[0] - '1';
    int col = move[2] - '1';
    int index = row * 3 + col;
    return index;
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

int check_win(const char board[9])
{
	// Check rows
	for (int i = 0; i < 3; i++)
	{
		if (board[i *3] == board[i *3 + 1] && board[i *3] == board[i *3 + 2] && board[i *3] != '.')
		{
			return 1;
		}
	}

	// Check columns
	for (int i = 0; i < 3; i++)
	{
		if (board[i] == board[i + 3] && board[i] == board[i + 6] && board[i] != '.')
		{
			return 1;
		}
	}

	// Check diagonals
	if (board[0] == board[4] && board[0] == board[8] && board[0] != '.')
	{
		return 1;
	}

	if (board[2] == board[4] && board[2] == board[6] && board[2] != '.')
	{
		return 1;
	}

	return 0;
}

int check_draw(const char board[9])
{
	for (int i = 0; i < 9; i++)
	{
		if (board[i] == '.')
		{
			return 0;
		}
	}

	return 1;
}