#include "const.h"
#include "connection.h"
#include "request.h"

const char MSG_220_CONNECT_OK[] = "220 Anonymous FTP server ready.\r\n";
const char MSG_331_PASS_REQUIRE[] = "331 Please specify the password.\r\n";
const char MSG_230_LOGIN_SUCC[] = "230 Login successful.\r\n";
const char MSG_215_UNIX_TYPE[] = "215 UNIX Type: L8\r\n";
const char MSG_221_GOODBYE[] = "221 Goodbye.\r\n";
const char MSG_530_NOT_LOGIN[] = "530 Not logged in.\r\n";

const char MSG_200_TEMPLATE[] = "220 %s\r\n";
const char MSG_504_TEMPLATE[] = "504 %s\r\n";
const char MSG_530_TEMPLATE[] = "530 %s\r\n";

const int NUM_VERBS = 17;
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

const int VERB_REQUIRE_PARAM[] = {
    1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1};

const verb_func VERB_FUNCS[] = {
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
