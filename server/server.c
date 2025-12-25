#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../common/ipc.h"
#include "../common/world.h"
#include "../common/randomWalk.h"
#include "../common/protocol.h"

int main(void) {
    srand(time(NULL));

    /*
    Position pos;
    pos.x = 1;
    pos.y = 1;
    Probabilities pr;
    pr.p_up = 0.25;
    pr.p_down = 0.25;
    pr.p_right = 0.25;
    pr.p_left = 0.25;

    const Position start = { 1, 1 };

    World world = createWorld(2, 2, 1, 1);
    placeObstacles(&world);
    const WalkResults res = randomWalkReplications(start, pr, 33, 3, world);

    destroyWorld(&world);

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
        const int r = ipc_server_recv(&srv, (char*)&req, sizeof(SimRequest));

        if (r <= 0) {
            printf("Receive failed.\n");
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
        placeObstacles(&world);

        if (req.wantPath == 1) {
            WalkResults res = randomWalkReplications(start, pr, req.maxSteps, req.replications, world);

            if (ipc_server_send(&srv, (char*)&res, sizeof(WalkResults)) <= 0) {
                printf("Send failed.\n");
                ipc_server_stop(&srv);
                return 1;
            }
        } else {
            WalkPathResult res = randomWalkWithPath(start, pr, req.maxSteps, world);

            if (ipc_server_send(&srv, (char*)&res, sizeof(WalkPathResult)) <= 0) {
                printf("Send failed.\n");
                ipc_server_stop(&srv);
                return 1;
            }
        }

        destroyWorld(&world);
        printf("\nServer replied with simulation result.\n");
    }

    ipc_server_stop(&srv);
    return 0;
}
