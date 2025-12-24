#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../common/ipc.h"
#include "../common/world.h"
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
        printf("Server start failed.\n");
        return 1;
    }
    printf("Server listening on port %s...\n", IPC_PORT);

    if (ipc_server_accept(&srv) != 0) {
        printf("Accept failed.\n");
        ipc_server_stop(&srv);
        return 1;
    }
    printf("Client connected.\n");

    while (1) {
        printf("Waiting for client request.\n");
        SimRequest req;
        int r = ipc_server_recv(&srv, (char*)&req, sizeof(SimRequest));

        if (r <= 0) {
            printf("Receive failed.\n");
            ipc_server_stop(&srv);
            return 1;
        } else if (r != sizeof(SimRequest)) {
            printf("Received wrong size.\n");
            ipc_server_stop(&srv);
            return 1;
        }

        printf("\nRequest received.\n");

        if (req.end == 1) {
            printf("End request received.\n");
            break;
        }

        printf("World = %dx%d\n", req.sizeX, req.sizeY);
        printf("Start = [%d, %d]\n", req.startX, req.startY);
        printf("K = %d, replications = %d\n", req.maxSteps, req.replications);

        //simulation
        const Position start = { req.startX, req.startY };
        const Probabilities pr = {
            req.p_up,
            req.p_down,
            req.p_left,
            req.p_right
        };
        World world = createWorld(req.sizeX, req.sizeY, req.startX, req.startY);
        placeObstacles(world);
        WalkResults res = randomWalkReplications(start, pr, req.maxSteps, req.replications, world);

        destroyWorld(&world);

        if (ipc_server_send(&srv, (char*)&res, sizeof(WalkResults)) <= 0) {
            printf("Send failed.\n");
            ipc_server_stop(&srv);
            return 1;
        }

        printf("\nServer replied with simulation result.\n");
    }

    ipc_server_stop(&srv);
    return 0;
}
