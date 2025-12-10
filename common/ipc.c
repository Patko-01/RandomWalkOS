#include "ipc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int ipc_init() {
    WSADATA wsaData;
    int r = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (r != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", r);
        return -1;
    }
    return 0;
}

void ipc_shutdown() {
    WSACleanup();
}

int ipc_server_start(ipc_server* srv, const char* port) {
    if (!srv || !port) {
        return -1;
    }
    srv->listen_sock = INVALID_SOCKET;
    srv->conn_sock = INVALID_SOCKET;

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        return -1;
    }

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = htons((unsigned short)atoi(port));

    if (bind(listenSock, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) {
        fprintf(stderr, "bind (localhost) failed: %d\n", WSAGetLastError());
        closesocket(listenSock);
        return -1;
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        closesocket(listenSock);
        return -1;
    }

    srv->listen_sock = listenSock;
    return 0;
}

int ipc_server_accept(ipc_server* srv) {
    if (!srv || srv->listen_sock == INVALID_SOCKET) return -1;
    SOCKET clientSock = accept(srv->listen_sock, NULL, NULL);
    if (clientSock == INVALID_SOCKET) {
        fprintf(stderr, "accept failed: %d\n", WSAGetLastError());
        return -1;
    }
    srv->conn_sock = clientSock;
    return 0;
}

int ipc_server_recv(ipc_server* srv, char* buf, size_t buf_size) {
    if (!srv || srv->conn_sock == INVALID_SOCKET || !buf) {
        return -1;
    }
    int r = recv(srv->conn_sock, buf, (int)buf_size, 0);
    if (r == SOCKET_ERROR) {
        return -1;
    }
    return r;
}

int ipc_server_send(ipc_server* srv, const char* buf, size_t len) {
    if (!srv || srv->conn_sock == INVALID_SOCKET || !buf) {
        return -1;
    }
    int s = send(srv->conn_sock, buf, (int)len, 0);
    if (s == SOCKET_ERROR) {
        return -1;
    }
    return s;
}

void ipc_server_stop(ipc_server* srv) {
    if (!srv) {
        return;
    }
    if (srv->conn_sock != INVALID_SOCKET) {
        closesocket(srv->conn_sock);
        srv->conn_sock = INVALID_SOCKET;
    }
    if (srv->listen_sock != INVALID_SOCKET) {
        closesocket(srv->listen_sock);
        srv->listen_sock = INVALID_SOCKET;
    }
}

int ipc_client_connect(ipc_client* cli, const char* port) {
    if (!cli || !port) {
        return -1;
    }
    cli->sock = INVALID_SOCKET;

    struct addrinfo hints = {0}, *result = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int rv = getaddrinfo("127.0.0.1", port, &hints, &result);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo failed: %d\n", rv);
        return -1;
    }

    SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        return -1;
    }
    if (connect(s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        fprintf(stderr, "connect failed: %d\n", WSAGetLastError());
        closesocket(s);
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);

    cli->sock = s;
    return 0;
}

int ipc_client_recv(ipc_client* cli, char* buf, size_t buf_size) {
    if (!cli || cli->sock == INVALID_SOCKET || !buf) {
        return -1;
    }
    int r = recv(cli->sock, buf, (int)buf_size, 0);
    if (r == SOCKET_ERROR) {
        return -1;
    }
    return r;
}

int ipc_client_send(ipc_client* cli, const char* buf, size_t len) {
    if (!cli || cli->sock == INVALID_SOCKET || !buf) {
        return -1;
    }
    int s = send(cli->sock, buf, (int)len, 0);
    if (s == SOCKET_ERROR) {
        return -1;
    }
    return s;
}

void ipc_client_close(ipc_client* cli) {
    if (!cli) {
        return;
    }
    if (cli->sock != INVALID_SOCKET) {
        closesocket(cli->sock);
        cli->sock = INVALID_SOCKET;
    }
}
