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
    int ConnectFD;
    int verb_idx;
    const char* verb;
    const char* param;

    enum UserState user_state;

    enum DataMode data_mode;
    struct sockaddr_in data_address;
    int dataSocketFD;
    int dataConnectFD;

    char root_path[BUFFER_SIZE];
    char current_path[BUFFER_SIZE];

    enum RNFRState RNFR_state;
    char RNFR_target[BUFFER_SIZE];

    int rest_position;
};

int write_message(int ConnectFD, const char *msg);
void *connection(void *arg);

#endif