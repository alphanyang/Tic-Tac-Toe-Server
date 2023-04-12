#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct {
    char *msg_type;
    int size;
    char *content;
} Message;

#define MSG_PLAY "PLAY"
#define MSG_NAME "NAME"
#define MSG_WAIT "WAIT"
#define MSG_BEGN "BEGN"
#define MSG_MOVE "MOVE"
#define MSG_MOVD "MOVD"
#define MSG_INVL "INVL"
#define MSG_RSGN "RSGN"
#define MSG_DRAW "DRAW"
#define MSG_OVER "OVER"

#define ROLE_X "X"
#define ROLE_O "O"

#define OUTCOME_WIN  "W"
#define OUTCOME_LOSS "L"
#define OUTCOME_DRAW "D"
#define OUTCOME_CONTINUE "C"

void send_message(int sockfd, const char *msg);
char *receive_message(int sockfd);
Message *parse_message(const char *msg);
char *format_message(const char *msg_type, ...);


char *game_state_string();
void update_board(char *board, const char *move, char *role);
int process_move(char player_role, int row, int col);
const char *get_board();
int validate_move(const char *move);
char game_over(const char *board);

// void lock_game_state();
// void unlock_game_state();
// void reset_game_state();

// char get_current_player();
// void set_player_socket(char role, int socket);
// void set_current_player(char role);
// int get_player_socket(char role);


#endif // PROTOCOL_H
