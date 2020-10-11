// Reference: https://en.wikipedia.org/wiki/Berkeley_sockets#Server

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <pthread.h>

#include "connection.h"
#include "const.h"

int SocketFD;

void SIGINT_handler(int x)
{
    close(SocketFD);
    exit(EXIT_SUCCESS);
}

int main(void)
{
    struct sockaddr_in sa;
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (SocketFD == -1)
    {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    memset(&sa, 0, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(FTP_SERVER_PORT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuse = 1;
    setsockopt(SocketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    if (bind(SocketFD, (struct sockaddr *)&sa, sizeof sa) == -1)
    {
        perror("bind failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    if (listen(SocketFD, 10) == -1)
    {
        perror("listen failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, SIGINT_handler);

    for (;;)
    {
        int ConnectFD = accept(SocketFD, NULL, NULL);

        if (0 > ConnectFD)
        {
            perror("accept failed");
            close(SocketFD);
            exit(EXIT_FAILURE);
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, connection, (void *)&ConnectFD);
    }

    close(SocketFD);
    return EXIT_SUCCESS;
}