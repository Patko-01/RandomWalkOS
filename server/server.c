#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../common/ipc.h"
#include "../common/world.h"
#include "../common/randomWalk.h"
#include "../common/protocol.h"

int main(void) {
    srand(time(NULL));
    /* testing
    Position pos;
    pos.x = 1;
    pos.y = 1;
    Probabilities pr;
    pr.p_up = 0.25;
    pr.p_down = 0.25;
    pr.p_right = 0.25;
    pr.p_left = 0.25;

    const Position start = { 1, 1 };

    World world = createWorld(2, 2);
    placeObstacles(&world);
    const WalkResult res = randomWalkReplications(start, pr, 33, 3, world);

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
        EndRequest endReq;
        const int er = ipc_server_recv(&srv, (char*)&endReq, sizeof(EndRequest));

        if (er <= 0) {
            printf("Receive failed (end).\n");
            ipc_server_stop(&srv);
            return 1;
        }
        if (endReq.end) {
            printf("End request received.\n");
            ipc_server_stop(&srv);
            return 0;
        }

        printf("--------------------------------\n");
        printf("Waiting for client mode request.\n");

        ModeRequest modeReq;
        const int mreq = ipc_server_recv(&srv, (char*)&modeReq, sizeof(ModeRequest));

        if (mreq <= 0) {
            printf("Receive failed (mode).\n");
            ipc_server_stop(&srv);
            return 1;
        }
        printf("Mode request received.\n");

        printf("\nWaiting for client map request.\n");

        MapRequest mapReq;
        const int mr = ipc_server_recv(&srv, (char*)&mapReq, sizeof(MapRequest));

        if (mr <= 0) {
            printf("Receive failed (map).\n");
            ipc_server_stop(&srv);
            return 1;
        }
        printf("Map request received.\n");

        World world = createWorld(mapReq.sizeX, mapReq.sizeY);
        if (mapReq.obstaclesMode == 1) {
            placeObstacles(&world);
        }

        StartPositionRequest startReq;
        if (modeReq.mode == 2) {
            while (1) {
                printf("\nWaiting for client starting position request.\n");

                const int sr = ipc_server_recv(&srv, (char*)&startReq, sizeof(StartPositionRequest));

                if (sr <= 0) {
                    printf("Receive failed (starting position).\n");
                    ipc_server_stop(&srv);
                    return 1;
                }

                printf("Starting position request received.\n");

                if (isSafeToStart(&world, startReq.startX, startReq.startY)) {
                    printf("Starting position is OK.\n");

                    StartPositionResult pr;
                    pr.notOk = 0;

                    if (ipc_server_send(&srv, (char*)&pr, sizeof(StartPositionResult)) <= 0) {
                        printf("Send failed (position request result).\n");
                        ipc_server_stop(&srv);
                        return 1;
                    }
                    break;
                } else {
                    printf("Starting position is not OK.\n");

                    StartPositionResult pr;
                    pr.notOk = 1;

                    if (ipc_server_send(&srv, (char*)&pr, sizeof(StartPositionResult)) <= 0) {
                        printf("Send failed (position request result).\n");
                        ipc_server_stop(&srv);
                        return 1;
                    }
                }
            }
        }

        printf("\nWaiting for client simulation request.\n");

        SimRequest req;
        const int r = ipc_server_recv(&srv, (char*)&req, sizeof(SimRequest));

        if (r <= 0) {
            printf("Receive failed.\n");
            ipc_server_stop(&srv);
            return 1;
        }

        printf("Simulation request received.\n");

        if (modeReq.mode == 1) {
            printf("Mode = summary\n");
        } else if (modeReq.mode == 2) {
            printf("Mode = interactive\n");
        }
        printf("World = %dx%d\n", mapReq.sizeX, mapReq.sizeY);

        if (modeReq.mode == 1) {
            printf("K = %d, replications = %d\n", req.maxSteps, req.replications);
        } else if (modeReq.mode == 2) {
            printf("K = %d\n", req.maxSteps);
        }

        const Probabilities pr = {
            req.p_up,
            req.p_down,
            req.p_left,
            req.p_right
        };

        if (modeReq.mode == 1) {
            WalkResult res[mapReq.sizeX][mapReq.sizeY];

            memset(res, 0, mapReq.sizeX * mapReq.sizeY * sizeof(WalkResult));

            for (int y = 0; y < mapReq.sizeY; y++) {
                for (int x = 0; x < mapReq.sizeX; x++) {
                    if (isSafeToStart(&world, x, y)) {
                        const Position start = { x, y };
                        res[x][y] = randomWalkReplications(start, pr, req.maxSteps, req.replications, world);
                    }
                }
            }

            if (ipc_server_send(&srv, (char*)&res, mapReq.sizeX * mapReq.sizeY * sizeof(WalkResult)) <= 0) {
                printf("Send failed (simulation result).\n");
                ipc_server_stop(&srv);
                return 1;
            }
        } else if (modeReq.mode == 2) {
            const Position start = { startReq.startX, startReq.startY };
            WalkPathResult res = randomWalkWithPath(start, pr, req.maxSteps, world);

            if (ipc_server_send(&srv, (char*)&res, sizeof(WalkPathResult)) <= 0) {
                printf("Send failed (simulation result).\n");
                ipc_server_stop(&srv);
                return 1;
            }
        }

        destroyWorld(&world);
        printf("\nServer replied with simulation result.\n");
    }
}
