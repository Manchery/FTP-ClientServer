#ifndef CONST
#define CONST

#define MAX_VERB_LEN 5

#define BUFFER_SIZE 2048

extern const char MSG_150_DATA_CONN_READY[];

extern const char MSG_200_PORT_OK[];
extern const char MSG_215_UNIX_TYPE[];
extern const char MSG_221_GOODBYE[];
extern const char MSG_226_TRANS_DONE[];
extern const char MSG_227_PASV_OK[];
extern const char MSG_230_LOGIN_SUCC[];
extern const char MSG_250_CWD_OK[];
extern const char MSG_250_RMD_OK[];
extern const char MSG_250_RNTO_OK[];
extern const char MSG_250_DELE_OK[];
extern const char MSG_257_MKD_OK[];
extern const char MSG_257_PWD_OK[];

extern const char MSG_331_PASS_REQUIRE[];
extern const char MSG_350_REQUIRE_MORE[];

extern const char MSG_425_NO_DATA_CONN[];
extern const char MSG_451_ACTION_FAIL[];

extern const char MSG_500_CMD_INVALID[];
extern const char MSG_501_PARAM_INVALID[];
extern const char MSG_503_RNTO_REQ_RNFR[];
extern const char MSG_504_TYPE_ERR[];
extern const char MSG_530_NOT_LOGIN[];
extern const char MSG_550_WRONG_PATH[];
extern const char MSG_550_NOT_FILE[];

extern const int NUM_VERBS;
extern const char *VERBS[];

extern const int VERB_REQUIRE_PARAM[];

struct ConnectionData;
typedef int (*verb_func)(struct ConnectionData *);
extern const verb_func VERB_FUNCS[];

extern char ROOT_DIR[BUFFER_SIZE];
extern int FTP_SERVER_PORT;

#endif