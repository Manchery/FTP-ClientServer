#include "commands.h"
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

// For verbs, to return 1 means it processes as expected

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
        write_message_by_code(connect->ConnectFD, 530, "This FTP server is anonymous only.");
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
    switch (connect->user_state)
    {
    case NOT_LOGIN:
        write_message_by_code(connect->ConnectFD, 530, "Need username.");
        break;
    case REQUIRE_PASS:
        write_message(connect->ConnectFD, MSG_230_LOGIN_SUCC);
        connect->user_state = LOGIN;
        break;
    case LOGIN:
        write_message_by_code(connect->ConnectFD, 530, "Already logged in.");
        break;
    }
    return 1;
}

// param format: h1,h2,h3,h4,p1,p2
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
        write_message_by_code(connect->ConnectFD, 501, "Invalid IP address or port.");
    }
    return 1;
}

int PASV(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    // drop connections already made
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
    connect->data_address.sin_port = htons(0);
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

    char msg_buffer[BUFFER_SIZE];
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

static int send_file(const char *filename, struct ConnectionData *connect)
{
    int FD = open(filename, O_RDONLY);
    if (FD == -1)
        return 0;

    if (connect->rest_position)
    {
        lseek(FD, connect->rest_position, SEEK_SET);
        connect->rest_position = 0;
    }

    char buffer[BUFFER_SIZE];
    for (;;)
    {
        int len = read(FD, buffer, BUFFER_SIZE);
        if (!len)
            break;
        if (write(connect->dataConnectFD, buffer, len)==-1)
        {
            return 0;
        }
    }
    write_message(connect->ConnectFD, MSG_226_TRANS_DONE);
    return 1;
}

static int recv_file(const char *filename, struct ConnectionData *connect)
{
    int FD = 0;
    if (connect->rest_position)
    {
        FD = open(filename, O_WRONLY | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
        if (FD == -1)
            return 0;
        lseek(FD, connect->rest_position, SEEK_SET);
        connect->rest_position = 0;
    }
    else
    {
        FD = open(filename, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        if (FD == -1)
            return 0;
    }

    char buffer[BUFFER_SIZE];
    for (;;)
    {
        int len = read(connect->dataConnectFD, buffer, BUFFER_SIZE);
        if (!len)
            break;
        write(FD, buffer, len);
    }
    write_message(connect->ConnectFD, MSG_226_TRANS_DONE);
    return 1;
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

// get absolute path and virtual path, using connect->param
static int get_absolute_path(char *virtual_path, char *absolute_path, struct ConnectionData *connect, int is_dir)
{
    strcpy(virtual_path, connect->current_path);
    if (connect->param)
    {
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
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFREG))
    {
        write_message_template(connect->ConnectFD, MSG_550_NOT_FILE, virtual_path);
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

    if (!send_file(absolute_path, connect))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "open file error.");
        return 0;
    }

    close_data_connection(connect);
    return 1;
}

int STOR(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
        return 0;
    }

    write_message(connect->ConnectFD, MSG_150_DATA_CONN_READY);

    if (!open_data_connection(connect))
    {
        return 0;
    }

    if (!recv_file(absolute_path, connect))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "open file error.");
        return 0;
    }

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

    if (strcmp("I", connect->param))
    {
        write_message(connect->ConnectFD, MSG_504_TYPE_ERR);
        return 1;
    }
    else
    {
        write_message_by_code(connect->ConnectFD, 200, "Type set to I.");
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
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
        return 0;
    }

    if (mkdir(absolute_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    {
        write_message_by_code(connect->ConnectFD, 550, "Directory creation failed.");
        return 1;
    }

    write_message_template(connect->ConnectFD, MSG_257_MKD_OK, virtual_path);
    return 1;
}

int CWD(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
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

    write_message_template(connect->ConnectFD, MSG_250_CWD_OK, connect->current_path);
    return 1;
}

int PWD(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    write_message_template(connect->ConnectFD, MSG_257_PWD_OK, connect->current_path);
    return 1;
}

int LIST(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
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
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "unknown.");
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
    if (!get_absolute_path(virtual_path, absolute_path, connect, 1))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFDIR))
    {
        write_message(connect->ConnectFD, MSG_550_WRONG_PATH);
        return 1;
    }

    if (startswith(connect->current_path, virtual_path))
    {
        // when remove current directory
        write_message_by_code(connect->ConnectFD, 550, "Cannot remove current directory.");
        return 1;
    }

    if (!rmdir(absolute_path))
    {
        write_message_template(connect->ConnectFD, MSG_250_RMD_OK, virtual_path);
    }
    else
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "unknown.");
    }
    return 1;
}

int DELE(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
        return 0;
    }

    struct stat s = {0};
    stat(absolute_path, &s);
    if (!(s.st_mode & S_IFREG))
    {
        write_message_template(connect->ConnectFD, MSG_550_NOT_FILE, virtual_path);
        return 1;
    }

    if (!remove(absolute_path))
    {
        write_message_template(connect->ConnectFD, MSG_250_DELE_OK, virtual_path);
    }
    else
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "unknown.");
    }

    return 1;
}

int RNFR(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
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
    write_message(connect->ConnectFD, MSG_350_REQUIRE_MORE);
    return 1;
}

int RNTO(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    if (connect->RNFR_state != RNFR_READY)
    {
        write_message(connect->ConnectFD, MSG_503_RNTO_REQ_RNFR);
        return 1;
    }
    else
    {
        connect->RNFR_state = NO_RNFR;
    }

    char virtual_path[BUFFER_SIZE], absolute_path[BUFFER_SIZE];
    if (!get_absolute_path(virtual_path, absolute_path, connect, 0))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "error when processing path.");
        return 0;
    }

    if (rename(connect->RNFR_target, absolute_path))
    {
        write_message_template(connect->ConnectFD, MSG_451_ACTION_FAIL, "unknown.");
    }
    else
    {
        write_message(connect->ConnectFD, MSG_250_RNTO_OK);
    }

    return 1;
}

int REST(struct ConnectionData *connect)
{
    if (!check_userstate(connect))
        return 0;

    int tmp;
    if (sscanf(connect->param, "%d", &tmp))
    {
        connect->rest_position = tmp;
        write_message(connect->ConnectFD, MSG_350_REQUIRE_MORE);
    }
    else
    {
        write_message(connect->ConnectFD, MSG_501_PARAM_INVALID);
    }

    return 1;
}