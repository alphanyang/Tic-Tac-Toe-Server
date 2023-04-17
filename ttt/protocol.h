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

void write_msg(int sock_fd, char *msg);
char *receive_msg(int sock_fd);
char *read_msg(int sock_fd);
Message *parse_message(const char *msg);
char *format_message(const char *msg_type, ...);
int parse_index(char *move);

char *game_state_string();
void update_board(char *board, const char *move, char *role);
const char *get_board();
int validate_move(const char *move);
int check_win(const char *board);
int check_draw(const char *board);

#endif // PROTOCOL_H
