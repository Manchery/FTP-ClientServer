#include "const.h"
#include "connection.h"
#include "commands.h"

const char MSG_150_DATA_CONN_READY[] = "150 File status okay; about to open data connection.\r\n";

const char MSG_200_PORT_OK[] = "200 PORT command successful.\r\n";
const char MSG_215_UNIX_TYPE[] = "215 UNIX Type: L8\r\n";
const char MSG_221_GOODBYE[] = "221 Goodbye.\r\n";
const char MSG_226_TRANS_DONE[] = "226 Transfer complete.\r\n";
const char MSG_227_PASV_OK[] = "227 Entering Passive Mode (%s,%d,%d)\r\n";
const char MSG_230_LOGIN_SUCC[] = "230 Login successful.\r\n";
const char MSG_250_CWD_OK[] = "250 Directory changed to \"%s\".\r\n";
const char MSG_250_RMD_OK[] = "250 \"%s\" directory removed.\r\n";
const char MSG_250_RNTO_OK[] = "250 Rename successful.\r\n";
const char MSG_250_DELE_OK[] = "250 Delete \"%s\" successful.\r\n";
const char MSG_257_MKD_OK[] = "257 \"%s\" directory created.\r\n";
const char MSG_257_PWD_OK[] = "257 \"%s\" is the current directory\r\n";

const char MSG_331_PASS_REQUIRE[] = "331 Guest login ok, send your complete e-mail address as password.\r\n";
const char MSG_350_REQUIRE_MORE[] = "350 Requested file action pending further information.\r\n";

const char MSG_425_NO_DATA_CONN[] = "425 No data connection.\r\n";
const char MSG_451_ACTION_FAIL[] = "451 Requested action aborted. Local error in processing. Detailed: %s\r\n";

const char MSG_500_CMD_INVALID[] = "500 Syntax error, command unrecognized.\r\n";
const char MSG_501_PARAM_INVALID[] = "501 Syntax error in parameters or arguments.\r\n";
const char MSG_503_RNTO_REQ_RNFR[] = "503 Bad sequence of commands.\r\n";
const char MSG_504_TYPE_ERR[] = "504 Only support TYPE I.\r\n";
const char MSG_530_NOT_LOGIN[] = "530 Not logged in.\r\n";
const char MSG_550_WRONG_PATH[] = "550 No such file or directory.\r\n";
const char MSG_550_NOT_FILE[] = "550 \"%s\" is not a file or does not exist.\r\n";

const int NUM_VERBS = 19;
const char *VERBS[] = {
    "USER", "PASS", "PORT", "PASV",
    "RETR", "STOR", "SYST", "TYPE",
    "QUIT", "ABOR", "MKD", "CWD",
    "PWD", "LIST", "RMD", "RNFR",
    "RNTO", "REST", "DELE"};

const int VERB_REQUIRE_PARAM[] = {
    1, 1, 1, 0,
    1, 1, 0, 1,
    0, 0, 1, 1,
    0, 0, 1, 1,
    1, 1, 1};

const verb_func VERB_FUNCS[] = {
    USER, PASS, PORT, PASV,
    RETR, STOR, SYST, TYPE,
    QUIT, ABOR, MKD, CWD,
    PWD, LIST, RMD, RNFR,
    RNTO, REST, DELE};

char ROOT_DIR[BUFFER_SIZE] = "/tmp/";
int FTP_SERVER_PORT = 21;