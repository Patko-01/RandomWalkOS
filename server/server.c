#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../common/ipc.h"
#include "../common/randomWalk.h"
#include "../common/protocol.h"

int main(void) {
    srand(time(NULL));
    /* test
    Position pos;
    pos.x = 1;
    pos.y = 23;
    Probabilities pr;
    pr.p_up = 0.24;
    pr.p_down = 0.26;
    pr.p_right = 0.25;
    pr.p_left = 0.25;
    WalkResult res = randomWalkReplications(pos, pr, 123, 2);

    printf("avg steps %lf, suc prob %lf\n", res.avgStepCount, res.probSuccess);
    */

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

    SimRequest req;
    int r = ipc_server_recv(&srv, (char*)&req, sizeof(SimRequest));
    if (r <= 0) {
        fprintf(stderr, "Recv failed\n");
        ipc_server_stop(&srv);
        return 1;
    } else if (r != sizeof(SimRequest)) {
        fprintf(stderr, "Received wrong size\n");
        ipc_server_stop(&srv);
        return 1;
    }

    printf("\nRequest received\n");

    printf("\nStart = [%d, %d]\n", req.startX, req.startY);
    printf("K = %d, reps = %d\n", req.maxSteps, req.replications);

    //simulation
    Position start = { req.startX, req.startY };
    Probabilities pr = {
        req.p_up,
        req.p_down,
        req.p_left,
        req.p_right
    };
    WalkResults res = randomWalkReplications(start, pr, req.maxSteps, req.replications);

    if (ipc_server_send(&srv, (char*)&res, sizeof(WalkResults)) <= 0) {
        fprintf(stderr, "Send failed\n");
        ipc_server_stop(&srv);
        return 1;
    }

    printf("\nServer replied with simulation result\n");

    ipc_server_stop(&srv);
    return 0;
}
