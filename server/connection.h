#ifndef CONNECTION
#define CONNECTION

#include "const.h"

enum UserState {
    NOT_LOGIN, REQUIRE_PASS, LOGIN
};

struct ConnectionData{
    int ConnectFD;
    int verb_idx;
    const char* verb;
    const char* param;

    enum UserState user_state;
};

int write_message(int ConnectFD, const char *msg);
void *connection(void *arg);

#endif