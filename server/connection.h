#ifndef CONNECTION
#define CONNECTION

#include "const.h"

struct ConnectionData{
    int ConnectFD;
    int verb_idx;
    const char* verb;
    const char* param;
};

void *connection(void *arg);

#endif