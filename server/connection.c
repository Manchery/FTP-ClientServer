#include "connection.h"
#include "const.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

void strupp(char *beg)
{
    while (*beg)
        *beg = toupper(*beg), beg++;
}

int write_message(int ConnectFD, const char *msg)
{
    write(ConnectFD, msg, strlen(msg));
    return 1;
}

static int parse_request(char *buffer, struct ConnectionData *connect)
{
    // assume buffer endswith \r\n
    char *space_ptr = strchr(buffer, (int)' ');
    int space_pos = -1;
    if (space_ptr)
    {
        space_pos = space_ptr - buffer;
    }

    int buffer_len = strlen(buffer);
    buffer[buffer_len - 2] = '\0';
    if (space_pos != -1)
    {
        buffer[space_pos] = '\0';
        connect->verb = buffer;
        connect->param = buffer + space_pos + 1;
    }
    else
    {
        connect->verb = buffer;
        connect->param = NULL;
    }

    strupp((char *)connect->verb);
    for (int i = 0; i < NUM_VERBS; i++)
    {
        if (strcmp(connect->verb, VERBS[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

void *connection(void *arg)
{
    int ConnectFD = *((int *)arg);

    struct ConnectionData connect;
    memset(&connect, 0, sizeof(connect));
    connect.ConnectFD = ConnectFD;

    char buffer[BUFFER_SIZE + 1];

    write_message(ConnectFD, MSG_220_CONNECT_OK);
    for (;;)
    {
        // TODO: multi segment (\r\n)
        int len = read(ConnectFD, buffer, BUFFER_SIZE);
        buffer[len] = '\0';
        // for (int i = 0; i < len; i++)
        // {
        //     printf("%d ", (int)buffer[i]);
        // }
        // printf("\n");
        if (len == 0)
        {
            break;
        }
        // TODO: check buffer endswith \r\n
        connect.verb_idx = parse_request(buffer, &connect);
        
        printf("%d\n%s\n%s\n", connect.verb_idx, connect.verb, connect.param);

        if (connect.verb_idx == -1)
        {
            perror("invalid verb");
            break;
        }
        if (VERB_REQUIRE_PARAM[connect.verb_idx] && !connect.param)
        {
            // TODO: print specific verb
            perror("request need a parameter");
            break;
        }

        VERB_FUNCS[connect.verb_idx](&connect);

        if (!strcmp(connect.verb, "QUIT"))
        {
            break;
        }
    }

    close(ConnectFD);
    return 0;
}