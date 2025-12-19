#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../common/ipc.h"
#include "../common/randomWalk.h"

int main() {
    srand(time(NULL));

    ipc_server srv;
    if (ipc_server_start(&srv, IPC_PORT) != 0) {
        fprintf(stderr, "Server start failed\n");
        return 1;
    }
    printf("Server listening on port %s...\n", IPC_PORT);

    if (ipc_server_accept(&srv) != 0) {
        fprintf(stderr, "Accept failed\n");
        ipc_server_stop(&srv);
        return 1;
    }
    printf("Client connected.\n");

    char buf[64] = {0};
    int r = ipc_server_recv(&srv, buf, sizeof(buf)-1);
    if (r <= 0) {
        fprintf(stderr, "Recv failed\n");
        ipc_server_stop(&srv);
        return 1;
    }
    buf[r] = '\0';
    printf("Server received: %s\n", buf);

    //simulation
    Position start = {1, 1};
    Probabilities p = {0.25, 0.25, 0.25, 0.25};
    WalkResult res = randomWalkReplications(start, p, 100, 5);
    printf("steps %f\n", res.avgStepCount);
    printf("probability %f\n", res.probSuccess);

    char reply[128];
    snprintf(reply, sizeof(reply), "avg=%.2f prob=%.2f", res.avgStepCount, res.probSuccess);

    if (ipc_server_send(&srv, reply, strlen(reply)) <= 0) {
        fprintf(stderr, "Send failed\n");
        ipc_server_stop(&srv);
        return 1;
    }

    printf("Server replied: %s\n", reply);

    ipc_server_stop(&srv);
    return 0;
}
