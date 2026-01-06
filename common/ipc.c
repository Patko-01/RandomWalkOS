#include "ipc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int ipcServerStart(ipcServer* srv, const char* port) {
    if (!srv || !port) {
        return -1;
    }
    srv->listenSock = -1;
    srv->connSock = -1;

    const int listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock < 0) {
        printf("\033[31mServer socket failed.\033[0m\n");
        return -1;
    }

    const int opt = 1;
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("\033[31mServer setsockopt(SO_REUSEADDR) failed.\033[0m\n");
        close(listenSock);
        return -1;
    }

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    sa.sin_port = htons((unsigned short)atoi(port));

    if (bind(listenSock, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        printf("\033[31mServer bind (localhost) failed.\033[0m\n");
        close(listenSock);
        return -1;
    }

    if (listen(listenSock, NUMBER_OF_CLIENTS) < 0) {
        printf("\033[31mServer listen failed.\033[0m\n");
        close(listenSock);
        return -1;
    }

    srv->listenSock = listenSock;
    return 0;
}

int ipcServerAccept(ipcServer* srv) {
    if (!srv || srv->listenSock < 0) {
        return -1;
    }
    const int clientSock = accept(srv->listenSock, NULL, NULL);
    if (clientSock < 0) {
        printf("\033[31mServer accept failed.\033[0m\n");
        return -1;
    }
    srv->connSock = clientSock;
    return 0;
}

int ipcServerRecv(ipcServer* srv, char* buf, const size_t bufSize) {
    if (!srv || srv->connSock < 0 || !buf) {
        return -1;
    }
    const ssize_t r = recv(srv->connSock, buf, bufSize, 0);
    if (r < 0) {
        return -1;
    }
    return (int)r;
}

int ipcServerSend(ipcServer* srv, const char* buf, const size_t len) {
    if (!srv || srv->connSock < 0 || !buf) {
        return -1;
    }
    const ssize_t s = send(srv->connSock, buf, len, 0);
    if (s < 0) {
        return -1;
    }
    return (int)s;
}

void ipcServerStop(ipcServer* srv) {
    if (!srv) {
        return;
    }
    if (srv->connSock >= 0) {
        close(srv->connSock);
        srv->connSock = -1;
    }
    if (srv->listenSock >= 0) {
        close(srv->listenSock);
        srv->listenSock = -1;
    }
}

int ipcClientConnect(ipcClient* cli, const char* port) {
    if (!cli || !port) {
        return -1;
    }
    cli->sock = -1;

    struct addrinfo hints;
    struct addrinfo* result = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const int rv = getaddrinfo("127.0.0.1", port, &hints, &result);
    if (rv != 0) {
        printf("\033[31mClient getaddrinfo failed.\033[0m\n");
        return -1;
    }

    const int s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s < 0) {
        printf("\033[31mClient socket failed.\033[0m\n");
        freeaddrinfo(result);
        return -1;
    }

    if (connect(s, result->ai_addr, result->ai_addrlen) < 0) {
        printf("\033[31mClient connect failed.\033[0m\n");
        close(s);
        freeaddrinfo(result);
        return -1;
    }

    freeaddrinfo(result);

    cli->sock = s;
    return 0;
}

int ipcClientRecv(ipcClient* cli, char* buf, const size_t bufSize) {
    if (!cli || cli->sock < 0 || !buf) {
        return -1;
    }
    const ssize_t r = recv(cli->sock, buf, bufSize, 0);
    if (r < 0) {
        return -1;
    }
    return (int)r;
}

int ipcClientSend(ipcClient* cli, const char* buf, const size_t len) {
    if (!cli || cli->sock < 0 || !buf) {
        return -1;
    }
    const ssize_t s = send(cli->sock, buf, len, 0);
    if (s < 0) {
        return -1;
    }
    return (int)s;
}

void ipcClientClose(ipcClient* cli) {
    if (!cli) {
        return;
    }
    if (cli->sock >= 0) {
        close(cli->sock);
        cli->sock = -1;
    }
}
