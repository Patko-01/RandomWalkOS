#ifndef PROJECTS_IPC_H
#define PROJECTS_IPC_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define IPC_PORT "13697"
#define NUMBER_OF_CLIENTS 1

// Socket types for server and client on POSIX/Linux
typedef struct ipc_server {
    int listen_sock;
    int conn_sock;
} ipc_server;

typedef struct ipc_client {
    int sock;
} ipc_client;

// Initialization / shutdown (no-ops on Linux, kept for API compatibility)
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

#endif