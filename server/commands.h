#ifndef REQUEST
#define REQUEST

#include "const.h"
#include "connection.h"

// For verbs, to return 1 means it processes as expected

int USER(struct ConnectionData *connect);
int PASS(struct ConnectionData *connect);
int PORT(struct ConnectionData *connect);
int PASV(struct ConnectionData *connect);
int RETR(struct ConnectionData *connect);
int STOR(struct ConnectionData *connect);
int SYST(struct ConnectionData *connect);
int TYPE(struct ConnectionData *connect);
int QUIT(struct ConnectionData *connect);
int ABOR(struct ConnectionData *connect);

int MKD(struct ConnectionData *connect);
int CWD(struct ConnectionData *connect);
int PWD(struct ConnectionData *connect);
int LIST(struct ConnectionData *connect);
int RMD(struct ConnectionData *connect);
int RNFR(struct ConnectionData *connect);
int RNTO(struct ConnectionData *connect);
int REST(struct ConnectionData *connect);

int DELE(struct ConnectionData *connect);

#endif