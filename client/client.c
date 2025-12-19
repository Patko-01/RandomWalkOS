#include <stdio.h>
#include <string.h>
#include "../common/ipc.h"

int main() {
    ipc_client cli;
    if (ipc_client_connect(&cli, IPC_PORT) != 0) {
        fprintf(stderr, "Client connect failed\n");
        return 1;
    }
    printf("Client connected to port %s.\n", IPC_PORT);

    const char* msg = "Hello";
    if (ipc_client_send(&cli, msg, strlen(msg)) <= 0) {
        fprintf(stderr, "Send failed\n");
        ipc_client_close(&cli);
        return 1;
    }
    printf("Client sent: %s\n", msg);

    char buf[64] = {0};
    int r = ipc_client_recv(&cli, buf, sizeof(buf)-1);
    if (r <= 0) {
        fprintf(stderr, "Recv failed\n");
        ipc_client_close(&cli);
        return 1;
    }
    buf[r] = '\0';
    printf("Client received: %s\n", buf);

    ipc_client_close(&cli);
    return 0;
}
