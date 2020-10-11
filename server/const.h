#ifndef CONST
#define CONST

#define FTP_SERVER_PORT 21
#define MAX_VERB_LEN 5

#define BUFFER_SIZE 2048

#define MAX_MSG_LEN 512
extern const char MSG_220_CONNECT_OK[];
extern const char MSG_331_PASS_REQUIRE[];
extern const char MSG_230_LOGIN_SUCC[];
extern const char MSG_215_UNIX_TYPE[];
extern const char MSG_221_GOODBYE[];
extern const char MSG_530_NOT_LOGIN[];

extern const char MSG_200_TEMPLATE[];
extern const char MSG_504_TEMPLATE[];
extern const char MSG_530_TEMPLATE[];

extern const int NUM_VERBS;
extern const char* VERBS[];

extern const int VERB_REQUIRE_PARAM[];

struct ConnectionData;
typedef int (*verb_func)(struct ConnectionData *);
extern const verb_func VERB_FUNCS[];

#endif