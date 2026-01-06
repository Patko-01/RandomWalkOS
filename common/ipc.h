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

typedef struct {
    int listenSock;
    int connSock;
} ipcServer;

typedef struct {
    int sock;
} ipcClient;

int ipcInit();
void ipcShutdown();

// Server
int ipcServerStart(ipcServer* srv, const char* port);
int ipcServerAccept(ipcServer* srv);
int ipcServerRecv(ipcServer* srv, char* buf, size_t bufSize);
int ipcServerSend(ipcServer* srv, const char* buf, size_t len);
void ipcServerStop(ipcServer* srv);

// Client
int ipcClientConnect(ipcClient* cli, const char* port);
int ipcClientRecv(ipcClient* cli, char* buf, size_t bufSize);
int ipcClientSend(ipcClient* cli, const char* buf, size_t len);
void ipcClientClose(ipcClient* cli);

#endif