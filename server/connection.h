#ifndef CONNECTION
#define CONNECTION

#include "commands.h"
#include <netinet/in.h>

enum UserState {
    NOT_LOGIN, REQUIRE_PASS, LOGIN
};

enum DataMode{
    NONE_MODE,
    PORT_MODE,
    PASV_MODE
};

enum RNFRState{
    NO_RNFR,
    RNFR_READY
};

struct ConnectionData{
    int ConnectFD;                      // connect socket
    int verb_idx;
    const char* verb;                   // command verb
    const char* param;                  // command parameters

    enum UserState user_state;

    enum DataMode data_mode;
    struct sockaddr_in data_address;
    int dataSocketFD;                   // data listen socket
    int dataConnectFD;                  // data connect socket

    char current_path[BUFFER_SIZE];     // virtual: under ROOT_DIR

    // for rename
    enum RNFRState RNFR_state;
    char RNFR_target[BUFFER_SIZE];

    // for resume transimission
    int rest_position;
};

int write_message(int ConnectFD, const char *msg);
int write_message_template(int ConnectFD, const char *template, const char *content);
int write_message_by_code(int ConnectFD, int code, const char *content);
void *connection(void *arg);

#endif