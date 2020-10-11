#ifndef CONST
#define CONST

#define FTP_SERVER_PORT 21
#define MAX_VERB_LEN 5

#define BUFFER_SIZE 2048

extern const char CONNECT_OK_MSG[];

extern const int NUM_VERBS;
extern const char* VERBS[];

struct ConnectionData;
typedef int (*verb_func)(struct ConnectionData *);
extern verb_func VERB_FUNCS[];

#endif