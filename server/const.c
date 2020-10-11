#include "const.h"
#include "connection.h"
#include "request.h"

const char CONNECT_OK_MSG[] = "220 ok\r\n";

const int NUM_VERBS = 10;
const char *VERBS[] = {
    "USER",
    "PASS",
    "PORT",
    "PASV",
    "RETR",
    "STOR",
    "SYST",
    "TYPE",
    "QUIT",
    "ABOR",
    "MKD",
    "CWD",
    "PWD",
    "LIST",
    "RMD",
    "RNFR",
    "RNTO"};

verb_func VERB_FUNCS[] = {
    USER,
    PASS,
    PORT,
    PASV,
    RETR,
    STOR,
    SYST,
    TYPE,
    QUIT,
    ABOR,
    MKD,
    CWD,
    PWD,
    LIST,
    RMD,
    RNFR,
    RNTO};
