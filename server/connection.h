#ifndef CONNECTION
#define CONNECTION

#include "const.h"
#include <netinet/in.h>

enum UserState {
    NOT_LOGIN, REQUIRE_PASS, LOGIN
};

enum DataMode{
    NONE,
    PORT_MODE,
    PASV_MODE
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

    char current_path[BUFFER_SIZE];
};

int write_message(int ConnectFD, const char *msg);
void *connection(void *arg);

#endif