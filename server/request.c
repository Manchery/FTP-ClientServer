#include "request.h"
#include "connection.h"
#include <string.h>

// TODO: carefully go over and check the code
static check_userstate(struct ConnectionData *connect)
{
    if (connect->user_state!=LOGIN){
        write_message(connect->ConnectFD, MSG_530_NOT_LOGIN);
        return 0;
    }
    return 1;
};

int USER(struct ConnectionData *connect)
{
    if (strcmp("anonymous", connect->param))
    {   
        char buffer[MAX_MSG_LEN];
        sprintf(buffer, MSG_530_TEMPLATE, "This FTP server is anonymous only.");
        write_message(connect->ConnectFD, buffer);
        return 1;
    }
    else
    {
        write_message(connect->ConnectFD, MSG_331_PASS_REQUIRE);
        connect->user_state = REQUIRE_PASS;
        return 1;
    }
}

int PASS(struct ConnectionData *connect)
{
    char buffer[MAX_MSG_LEN];
    switch (connect->user_state)
    {
    case NOT_LOGIN:
        sprintf(buffer, MSG_530_TEMPLATE, "Need username.");
        write_message(connect->ConnectFD, buffer);
        break;
    case REQUIRE_PASS:
        write_message(connect->ConnectFD, MSG_230_LOGIN_SUCC);
        connect->user_state = LOGIN;
        break;
    case LOGIN:
        sprintf(buffer, MSG_530_TEMPLATE, "Already logged in.");
        write_message(connect->ConnectFD, MSG_230_LOGIN_SUCC);
        break;
    }
    return 1;
}

int PORT(struct ConnectionData *connect)
{
    if (!check_userstate(connect)) return 0;
}
int PASV(struct ConnectionData *connect)
{
    if (!check_userstate(connect)) return 0;
}
int RETR(struct ConnectionData *connect)
{
    if (!check_userstate(connect)) return 0;
}
int STOR(struct ConnectionData *connect)
{
    if (!check_userstate(connect)) return 0;
}

int SYST(struct ConnectionData *connect)
{
    write_message(connect->ConnectFD, MSG_215_UNIX_TYPE);
    return 1;
}

int TYPE(struct ConnectionData *connect)
{
    if (!check_userstate(connect)) return 0;

    char buffer[MAX_MSG_LEN];
    if (strcmp("I", connect->param))
    {
        sprintf(buffer, MSG_504_TEMPLATE, "Only support TYPE I");
        write_message(connect->ConnectFD, buffer);
        return 1;
    }else{
        sprintf(buffer, MSG_200_TEMPLATE, "Type set to I.");
        write_message(connect->ConnectFD, buffer);
        return 1;
    }
}

int QUIT(struct ConnectionData *connect)
{
    write_message(connect->ConnectFD, MSG_221_GOODBYE);
    return 1;
}
int ABOR(struct ConnectionData *connect)
{
    QUIT(connect);
    return 1;
}

int MKD(struct ConnectionData *connect) {
    if (!check_userstate(connect)) return 0;
}
int CWD(struct ConnectionData *connect) {
    if (!check_userstate(connect)) return 0;
}
int PWD(struct ConnectionData *connect) {
    if (!check_userstate(connect)) return 0;
}
int LIST(struct ConnectionData *connect) {
    if (!check_userstate(connect)) return 0;
}
int RMD(struct ConnectionData *connect) {
    if (!check_userstate(connect)) return 0;
}
int RNFR(struct ConnectionData *connect) {
    if (!check_userstate(connect)) return 0;
}
int RNTO(struct ConnectionData *connect) {
    if (!check_userstate(connect)) return 0;
}