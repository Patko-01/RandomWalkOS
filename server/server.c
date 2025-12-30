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

    ipc_server srv;
    if (ipc_server_start(&srv, IPC_PORT) != 0) {
        printf("\033[31mServer start failed.\033[0m\n");
        return 1;
    }
    printf("Server listening on port %s...\n", IPC_PORT);

    if (ipc_server_accept(&srv) != 0) {
        printf("\033[31mAccept failed.\033[0m\n");
        ipc_server_stop(&srv);
        return 1;
    }
    printf("Client connected.\n");

    int worldExists = 0;

    while (1) {
        printf("--------------------------------------\n");
        printf("Server waiting for message header...\n");

        MessageHeader h;
        const int hr = ipc_server_recv(&srv, (char*)&h, sizeof(MessageHeader));
        if (hr <= 0) {
            printf("\033[31mReceive failed (header).\033[0m\n");
            ipc_server_stop(&srv);
            return 1;
        }

        printf("Server received message header.\n");

        ModeRequest modeReq;
        MapRequest mapReq;
        StartPositionRequest startReq;
        SimRequest req;

        World world;

        switch (h.type) {
            case MSG_EXIT:
                printf("Client requested exit.\n");
                destroyWorld(&world);
                ipc_server_stop(&srv);
                return 0;

            case MSG_MODE:
                printf("Waiting for client mode request...\n");

                const int modeR = ipc_server_recv(&srv, (char*)&modeReq, sizeof(ModeRequest));

                if (modeR <= 0) {
                    printf("\033[31mReceive failed (mode).\033[0m\n");
                    ipc_server_stop(&srv);
                    return 1;
                }
                printf("Mode request received.\n");
                break;

            case MSG_MAP:
                printf("\nWaiting for client map request...\n");

                const int mapR = ipc_server_recv(&srv, (char*)&mapReq, sizeof(MapRequest));

                if (mapR <= 0) {
                    printf("\033[31mReceive failed (map).\033[0m\n");
                    ipc_server_stop(&srv);
                    return 1;
                }
                printf("Map request received.\n");

                world = createWorld(mapReq.sizeX, mapReq.sizeY);
                if (mapReq.obstaclesMode == 1) {
                    placeObstacles(&world);
                }
                worldExists = 1;
                break;

            case MSG_START_POS:
                while (1) {
                    printf("\nWaiting for client starting position request...\n");

                    const int sr = ipc_server_recv(&srv, (char*)&startReq, sizeof(StartPositionRequest));

                    if (sr <= 0) {
                        printf("\033[31mReceive failed (starting position).\033[0m\n");
                        destroyWorld(&world);
                        ipc_server_stop(&srv);
                        return 1;
                    }

                    printf("Starting position request received.\n");

                    if (!worldExists) { // teoreticky toto nikdy nenastane v tejto situacii
                        world = createWorld(2, 2);
                    }

                    if (isSafeToStart(&world, startReq.startX, startReq.startY)) {
                        printf("Starting position is OK.\n");

                        StartPositionResult pr;
                        pr.notOk = 0;

                        if (ipc_server_send(&srv, (char*)&pr, sizeof(StartPositionResult)) <= 0) {
                            printf("\033[31mReceive failed (position request result).\033[0m\n");
                            destroyWorld(&world);
                            ipc_server_stop(&srv);
                            return 1;
                        }
                        break;
                    } else {
                        printf("Starting position is not OK.\n");

                        StartPositionResult pr;
                        pr.notOk = 1;

                        if (ipc_server_send(&srv, (char*)&pr, sizeof(StartPositionResult)) <= 0) {
                            printf("\033[31mReceive failed (position request result).\033[0m\n");
                            destroyWorld(&world);
                            ipc_server_stop(&srv);
                            return 1;
                        }
                    }
                }
                break;

            case MSG_SIMULATION:
                printf("\nWaiting for client simulation request...\n");

                const int r = ipc_server_recv(&srv, (char*)&req, sizeof(SimRequest));

                if (r <= 0) {
                    printf("\033[31mReceive failed (simulation).\033[0m\n");
                    destroyWorld(&world);
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
                                res[x][y] = randomWalkReplications(start, pr, req.maxSteps, req.replications, &world);
                            }
                        }
                    }

                    if (ipc_server_send(&srv, (char*)&res, mapReq.sizeX * mapReq.sizeY * sizeof(WalkResult)) <= 0) {
                        printf("\033[31mSend failed (simulation result).\033[0m\n");
                        destroyWorld(&world);
                        ipc_server_stop(&srv);
                        return 1;
                    }
                } else if (modeReq.mode == 2) {
                    if (!worldExists) { // teoreticky toto nikdy nenastane v tejto situacii
                        world = createWorld(2, 2);
                    }

                    const Position start = { startReq.startX, startReq.startY };
                    WalkPathResult res = randomWalkWithPath(start, pr, req.maxSteps, &world);

                    if (ipc_server_send(&srv, (char*)&res, sizeof(WalkPathResult)) <= 0) {
                        printf("\033[31mSend failed (simulation result).\033[0m\n");
                        destroyWorld(&world);
                        ipc_server_stop(&srv);
                        return 1;
                    }
                }

                destroyWorld(&world);
                printf("\nServer replied with simulation result.\n");
                break;
        }
    }
}
