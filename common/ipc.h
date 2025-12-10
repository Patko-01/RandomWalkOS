#ifndef SEMESTRALKA_IPC_H
#define SEMESTRALKA_IPC_H

#include <winsock2.h>
#include <ws2tcpip.h>

#define IPC_PORT "13697"

// Opaque types for server and client sockets
typedef struct ipc_server {
    SOCKET listen_sock;
    SOCKET conn_sock;
} ipc_server;

typedef struct ipc_client {
    SOCKET sock;
} ipc_client;

int ipc_init();
void ipc_shutdown();

// Server side
int ipc_server_start(ipc_server* srv, const char* port);
int ipc_server_accept(ipc_server* srv);
int ipc_server_recv(ipc_server* srv, char* buf, size_t buf_size);
int ipc_server_send(ipc_server* srv, const char* buf, size_t len);
void ipc_server_stop(ipc_server* srv);

// Client side
int ipc_client_connect(ipc_client* cli, const char* port);
int ipc_client_recv(ipc_client* cli, char* buf, size_t buf_size);
int ipc_client_send(ipc_client* cli, const char* buf, size_t len);
void ipc_client_close(ipc_client* cli);

#endif // SEMESTRALKA_IPC_H
