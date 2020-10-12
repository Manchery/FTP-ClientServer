#include "request.h"
#include "connection.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

// TODO: carefully go over and check the code
static int check_userstate(struct ConnectionData *connect)
{
    if (connect->user_state != LOGIN)
    {
        write_message(connect->ConnectFD, MSG_530_NOT_LOGIN);
        return 0;
    }
    return 1;
};

int USER(struct ConnectionData *connect)
{
    if (strcmp("anonymous", connect->param))
    {
        char msg_buffer[MAX_MSG_LEN];
        sprintf(msg_buffer, MSG_530_TEMPLATE, "This FTP server is anonymous only.");
        write_message(connect->ConnectFD, msg_buffer);
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
    char msg_buffer[MAX_MSG_LEN];
    switch (connect->user_state)
    {
    case NOT_LOGIN:
        sprintf(msg_buffer, MSG_530_TEMPLATE, "Need username.");
        write_message(connect->ConnectFD, msg_buffer);
        break;
    case REQUIRE_PASS:
        write_message(connect->ConnectFD, MSG_230_LOGIN_SUCC);
        connect->user_state = LOGIN;
        break;
    case LOGIN:
        sprintf(msg_buffer, MSG_530_TEMPLATE, "Already logged in.");
        write_message(connect->ConnectFD, msg_buffer);
        break;
    }
    return 1;
}

static int parse_data_address(struct sockaddr_in *data_address, const char *param)
{
    int len = strlen(param);
    int numbers[6] = {0}, number_count = 0;
    for (int i = 0; i < len; i++)
    {
        if (param[i] == ',')
        {
            number_count++;
            if (number_count >= 6)
                return 0;
        }
        else if (isdigit(param[i]))
        {
            numbers[number_count] *= 10;
            numbers[number_count] += param[i] - '0';
        }
        else
        {
            return 0;
        }
    }

    if (number_count != 5)
        return 0;
    for (int i = 0; i < number_count; i++)
        if (numbers[i] >= 256)
            return 0;

    bzero(data_address, sizeof(struct sockaddr_in));
    data_address->sin_family = AF_INET;
    data_address->sin_port = htons(numbers[4] * 256 + numbers[5]);
    char ip_buffer[20];
    sprintf(ip_buffer, "%d.%d.%d.%d", numbers[0], numbers[1], numbers[2], numbers[3]);
    data_address->sin_addr.s_addr = inet_addr(ip_buffer);
    return 1;
}

int PORT(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    if (connect->data_mode == PASV_MODE)
    {
        close(connect->dataSocketFD);
    }

    struct sockaddr_in data_address;
    if (parse_data_address(&data_address, connect->param))
    {
        connect->data_mode = PORT_MODE;
        connect->data_address = data_address;
        write_message(connect->ConnectFD, MSG_200_PORT_OK);
    }
    else
    {
        char msg_buffer[MAX_MSG_LEN];
        sprintf(msg_buffer, MSG_501_TEMPLATE, "Invalid IP address or port.");
        write_message(connect->ConnectFD, msg_buffer);
    }
    return 1;
}

// Reference: [c++ - Get the IP address of the machine - Stack Overflow](https://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine)
void GetPrimaryIp(char *buffer, size_t buflen)
{
    assert(buflen >= 16);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock != -1);

    const char *kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    int err = connect(sock, (const struct sockaddr *)&serv, sizeof(serv));
    assert(err != -1);

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr *)&name, &namelen);
    assert(err != -1);

    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);
    assert(p);

    close(sock);
}

int PASV(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    // TODO: drop connections already made
    if (connect->data_mode == PASV_MODE)
    {
        close(connect->dataSocketFD);
    }

    connect->data_mode = PASV_MODE;
    connect->dataSocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    // parse ip and port
    // refs:
    // [1] https://stackoverflow.com/questions/15914790/how-to-get-its-own-ip-address-with-a-socket-address
    // [2] https://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine

    bzero(&(connect->data_address), sizeof(connect->data_address));
    connect->data_address.sin_family = AF_INET;
    connect->data_address.sin_port = htons(0); // TODO: why 0?
    connect->data_address.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuse = 1;
    setsockopt(connect->dataSocketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    bind(connect->dataSocketFD, (struct sockaddr *)&(connect->data_address), sizeof(connect->data_address));
    listen(connect->dataSocketFD, 1);

    socklen_t namelen = sizeof(connect->data_address);
    getsockname(connect->dataSocketFD, (struct sockaddr *)&(connect->data_address), &namelen);
    int port = (int)(ntohs((connect->data_address).sin_port));

    char ip[20] = {0}, ip_len = 20;
    GetPrimaryIp(ip, ip_len);

    for (int i = 0; i < ip_len; i++)
        if (ip[i] == '.')
            ip[i] = ',';

    char msg_buffer[MAX_MSG_LEN];
    sprintf(msg_buffer, MSG_227_PASV_OK, ip, port / 256, port % 256);
    write_message(connect->ConnectFD, msg_buffer);

    return 1;
}

static int open_data_connection(struct ConnectionData *_connect)
{
    if (_connect->data_mode == NONE_MODE)
    {
        write_message(_connect->ConnectFD, MSG_425_NO_DATA_CONN);
        return 0;
    }
    if (_connect->data_mode == PASV_MODE)
    {
        _connect->dataConnectFD = accept(_connect->dataSocketFD, NULL, NULL);
    }
    if (_connect->data_mode == PORT_MODE)
    {
        // https://en.wikipedia.org/wiki/Berkeley_sockets#Client
        _connect->dataConnectFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (_connect->dataConnectFD == -1)
        {
            perror("cannot create socket");
            // TODO: more detailed message
            write_message(_connect->ConnectFD, MSG_425_NO_DATA_CONN);
            return 0;
        }
        if (connect(_connect->dataConnectFD, (struct sockaddr *)&(_connect->data_address), sizeof(_connect->data_address)) == -1)
        {
            perror("connect failed");
            close(_connect->dataConnectFD);
            write_message(_connect->ConnectFD, MSG_425_NO_DATA_CONN);
            return 0;
        }
    }
    return 1;
}

static void close_data_connection(struct ConnectionData *connect)
{
    if (connect->data_mode == PASV_MODE)
    {
        close(connect->dataSocketFD);
        close(connect->dataConnectFD);
    }
    if (connect->data_mode == PORT_MODE)
    {
        close(connect->dataConnectFD);
    }
    connect->data_mode = NONE_MODE;
}

static void send_file(const char *filename, struct ConnectionData *connect)
{
    int FD = open(filename, O_RDONLY);
    // TODO: rest position
    char buffer[BUFFER_SIZE];
    for (;;)
    {
        int len = read(FD, buffer, BUFFER_SIZE);
        if (!len)
            break;
        write(connect->dataConnectFD, buffer, len);
    }
    write_message(connect->ConnectFD, MSG_226_TRANS_DONE);
}

static void recv_file(const char *filename, struct ConnectionData *connect)
{
    int FD = open(filename, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    // TODO: rest position
    char buffer[BUFFER_SIZE];
    for (;;)
    {
        int len = read(connect->dataConnectFD, buffer, BUFFER_SIZE);
        if (!len)
            break;
        write(FD, buffer, len);
    }
    write_message(connect->ConnectFD, MSG_226_TRANS_DONE);
}

// Reference: [linux 下用 c 实现 ls -l 命令 - Ritchie丶 - 博客园](https://www.cnblogs.com/Ritchie/p/6272693.html)
static int send_ls(const char *dirname, struct ConnectionData *connect)
{
    DIR *dir_ptr;
    struct dirent *direntp;

    if ((dir_ptr = opendir(dirname)) == NULL)
    {
        return 0;
    }
    else
    {
        while ((direntp = readdir(dir_ptr)) != NULL)
        {
            struct stat info;
            char absolute_path[BUFFER_SIZE];
            strcpy(absolute_path, dirname);
            push_path(absolute_path, direntp->d_name, 0);

            if (stat(absolute_path, &info) == -1)
                return 0;
            else
            {
                char buffer[BUFFER_SIZE];
                show_file_info(buffer, absolute_path, direntp->d_name, &info);
                int len = strlen(buffer);
                write(connect->dataConnectFD, buffer, len);
            }
        }
        write_message(connect->ConnectFD, MSG_226_TRANS_DONE);
        closedir(dir_ptr);
        return 1;
    }
}

static int get_absolute_path(char *virtual_path, char *absolute_path, struct ConnectionData *connect, int is_dir){
    // TODO: out of length
    strcpy(virtual_path, connect->current_path);
    if (connect->param){
        if (!push_path(virtual_path, connect->param, is_dir))
        {
            write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
            return 0;
        }
    }

    strcpy(absolute_path, ROOT_DIR);
    if (!push_path(absolute_path, virtual_path + 1, is_dir))
    {
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 0;
    }

    return 1;
}

int RETR(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0)){
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFREG))
    {
        // TODO: wrong message content
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }
    else
    {
        write_message(connect->ConnectFD, MSG_150_DATA_CONN_READY);
    }

    if (!open_data_connection(connect))
    {
        return 0;
    }

    send_file(absolute_path, connect);

    close_data_connection(connect);
    return 1;
}

int STOR(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0)){
        return 0;
    }

    write_message(connect->ConnectFD, MSG_150_DATA_CONN_READY);

    if (!open_data_connection(connect))
    {
        return 0;
    }

    recv_file(absolute_path, connect);

    close_data_connection(connect);

    return 1;
}

int SYST(struct ConnectionData *connect)
{
    write_message(connect->ConnectFD, MSG_215_UNIX_TYPE);
    return 1;
}

int TYPE(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char msg_buffer[MAX_MSG_LEN];
    if (strcmp("I", connect->param))
    {
        sprintf(msg_buffer, MSG_504_TEMPLATE, "Only support TYPE I");
        write_message(connect->ConnectFD, msg_buffer);
        return 1;
    }
    else
    {
        sprintf(msg_buffer, MSG_200_TEMPLATE, "Type set to I.");
        write_message(connect->ConnectFD, msg_buffer);
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

int MKD(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1)){
        return 0;
    }

    if (mkdir(absolute_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    {
        // TODO: file already exists: wrong message: "550 No such file or directory."
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }

    char msg_buffer[MAX_MSG_LEN];
    sprintf(msg_buffer, MSG_257_MKD, virtual_path);
    write_message(connect->ConnectFD, msg_buffer);
    return 1;
}
int CWD(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1)){
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFDIR))
    {
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }

    strcpy(connect->current_path, virtual_path);

    char msg_buffer[MAX_MSG_LEN];
    sprintf(msg_buffer, MSG_250_CWD, connect->current_path);
    write_message(connect->ConnectFD, msg_buffer);
    return 1;
}
int PWD(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char msg_buffer[MAX_MSG_LEN];
    sprintf(msg_buffer, MSG_257_PWD, connect->current_path);
    write_message(connect->ConnectFD, msg_buffer);
    return 1;
}
int LIST(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1)){
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFDIR))
    {
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }
    else
    {
        write_message(connect->ConnectFD, MSG_150_DATA_CONN_READY);
    }

    if (!open_data_connection(connect))
    {
        return 0;
    }

    if (!send_ls(absolute_path, connect))
    {
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        close_data_connection(connect);
        return 1;
    }
    else
    {
        close_data_connection(connect);
        return 1;
    }
}
int RMD(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1)){
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFDIR))
    {
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }

    if (startswith(connect->current_path, virtual_path)){ // when remove current directory
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }

    if (!rmdir(absolute_path))
	{
        char msg_buffer[MAX_MSG_LEN];
        sprintf(msg_buffer, MSG_250_RMD, virtual_path);
		write_message(connect->ConnectFD, msg_buffer);
	}
	else
	{
		write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
	}
    return 1;
}
int RNFR(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0)){
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFDIR) && !(s.st_mode & S_IFREG))
    {
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }

    connect->RNFR_state = RNFR_READY;
    strcpy(connect->RNFR_target, absolute_path);
    write_message(connect->ConnectFD, MSG_350_RNFR);
    return 1;
}
int RNTO(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    if (connect->RNFR_state != RNFR_READY){
        write_message(connect->ConnectFD, MSG_503_RNTO_REQ_RNFR);
        return 1;
    }else{
        connect->RNFR_state = NO_RNFR;
    }

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0)){
        return 0;
    }

    if (rename(connect->RNFR_target, absolute_path))
	{
		write_message(connect->ConnectFD, MSG_451_RNTO_ERR);
	}
	else
	{
		write_message(connect->ConnectFD, MSG_250_RNTO_SUCC);
	}

    return 1;
}
int REST(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;
    return 1;
}