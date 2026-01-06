#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../common/ipc.h"
#include "../common/world.h"
#include "../common/printer.h"
#include "../common/protocol.h"
#include "../common/randomWalk.h"

int main(void) {
    srand((unsigned) time(NULL) ^ (unsigned) getpid()); //kod od AI

    ipcServer srv;
    if (ipcServerStart(&srv, IPC_PORT) != 0) {
        printf("\033[31mServer start failed.\033[0m\n");
        return 1;
    }
    printf("Server listening on port %s...\n", IPC_PORT);

    if (ipcServerAccept(&srv) != 0) {
        printf("\033[31mAccept failed.\033[0m\n");
        ipcServerStop(&srv);
        return 1;
    }
    printf("Client connected.\n");

    ModeRequest modeReq;
    MapRequest mapReq;
    StartPositionRequest startReq;
    SimRequest req;
    FileRequest fReq;
    World world;

    int load = 0;

    while (1) {
        printf("--------------------------------------\n");
        printf("Server waiting for message header...\n");

        MessageHeader h;
        const int hr = ipcServerRecv(&srv, (char *) &h, sizeof(MessageHeader));
        if (hr <= 0) {
            printf("\033[31mReceive failed (header).\033[0m\n");
            ipcServerStop(&srv);
            return 1;
        }

        printf("Server received message header.\n");

        switch (h.type) {
            case MSG_EXIT: {
                printf("Client requested exit.\n");
                destroyWorld(&world);
                ipcServerStop(&srv);
                return 0;
            }
            case MSG_MODE: {
                printf("\nWaiting for client mode request...\n");

                const int modeR = ipcServerRecv(&srv, (char *) &modeReq, sizeof(ModeRequest));

                if (modeR <= 0) {
                    printf("\033[31mReceive failed (mode).\033[0m\n");
                    ipcServerStop(&srv);
                    return 1;
                }
                printf("Mode request received.\n");
                break;
            }
            case MSG_MAP: {
                printf("\nWaiting for client map request...\n");

                const int mapR = ipcServerRecv(&srv, (char *) &mapReq, sizeof(MapRequest));

                if (mapR <= 0) {
                    printf("\033[31mReceive failed (map).\033[0m\n");
                    ipcServerStop(&srv);
                    return 1;
                }
                printf("Map request received.\n");

                world = createWorld(mapReq.sizeX, mapReq.sizeY);
                if (mapReq.obstaclesMode == 1) {
                    placeObstacles(&world);
                }

                WorldRequest wReq;
                for (int y = 0; y < world.sizeY; ++y) {
                    for (int x = 0; x < world.sizeX; ++x) {
                        wReq.world[x][y] = WORLD_AT(&world, x, y);
                    }
                }

                if (ipcServerSend(&srv, (char *) &wReq, sizeof(WorldRequest)) <= 0) {
                    printf("\033[31mSend failed (world generation result).\033[0m\n");
                    destroyWorld(&world);
                    ipcServerStop(&srv);
                    return 1;
                }
                break;
            }
            case MSG_START_POS: {
                printf("\nWaiting for client starting position request...\n");

                const int sr = ipcServerRecv(&srv, (char *) &startReq, sizeof(StartPositionRequest));

                if (sr <= 0) {
                    printf("\033[31mReceive failed (starting position).\033[0m\n");
                    destroyWorld(&world);
                    ipcServerStop(&srv);
                    return 1;
                }

                printf("Starting position request received.\n");

                if (isSafeToStart(&world, startReq.startX, startReq.startY)) {
                    printf("Starting position is OK.\n");

                    StartPositionResult pr;
                    pr.notOk = 0;

                    if (ipcServerSend(&srv, (char *) &pr, sizeof(StartPositionResult)) <= 0) {
                        printf("\033[31mSend failed (position request result).\033[0m\n");
                        destroyWorld(&world);
                        ipcServerStop(&srv);
                        return 1;
                    }
                } else {
                    printf("Starting position is not OK.\n");

                    StartPositionResult pr;
                    pr.notOk = 1;

                    if (ipcServerSend(&srv, (char *) &pr, sizeof(StartPositionResult)) <= 0) {
                        printf("\033[31mSend failed (position request result).\033[0m\n");
                        destroyWorld(&world);
                        ipcServerStop(&srv);
                        return 1;
                    }
                }
                break;
            }
            case MSG_SIMULATION: {
                if (!load) {
                    printf("\nWaiting for client simulation request...\n");

                    const int r = ipcServerRecv(&srv, (char *) &req, sizeof(SimRequest));

                    if (r <= 0) {
                        printf("\033[31mReceive failed (simulation).\033[0m\n");
                        destroyWorld(&world);
                        ipcServerStop(&srv);
                        return 1;
                    }
                }

                ReplicationRequest repReq;
                if (modeReq.mode == 1) {
                    const int r = ipcServerRecv(&srv, (char *) &repReq, sizeof(ReplicationRequest));

                    if (r <= 0) {
                        printf("\033[31mReceive failed (replication count).\033[0m\n");
                        destroyWorld(&world);
                        ipcServerStop(&srv);
                        return 1;
                    }
                }

                printf("Simulation request received.\n");

                if (modeReq.mode == 1) {
                    printf("Mode = summary\n");
                    printf("K = %d, replications = %d\n", req.maxSteps, repReq.replications);
                } else if (modeReq.mode == 2) {
                    printf("Mode = interactive\n");
                    printf("K = %d\n", req.maxSteps);
                }
                printf("World = %dx%d\n", mapReq.sizeX, mapReq.sizeY);

                const Probabilities pr = {
                    req.p_up,
                    req.p_down,
                    req.p_left,
                    req.p_right
                };

                WalkResult resRep[mapReq.sizeX][mapReq.sizeY];
                WalkPathResult resPath;

                if (modeReq.mode == 1) {
                    for (int y = 0; y < mapReq.sizeY; y++) {
                        for (int x = 0; x < mapReq.sizeX; x++) {
                            if (isSafeToStart(&world, x, y)) {
                                const Position start = {x, y};
                                resRep[x][y] = randomWalkReplications(start, pr, req.maxSteps, repReq.replications, &world);
                            }
                        }
                    }

                    if (ipcServerSend(&srv, (char *) &resRep, mapReq.sizeX * mapReq.sizeY * sizeof(WalkResult)) <= 0) {
                        printf("\033[31mSend failed (simulation result).\033[0m\n");
                        destroyWorld(&world);
                        ipcServerStop(&srv);
                        return 1;
                    }
                } else if (modeReq.mode == 2) {
                    const Position start = {startReq.startX, startReq.startY};
                    resPath = randomWalkWithPath(start, pr, req.maxSteps, &world);

                    if (ipcServerSend(&srv, (char *) &resPath, sizeof(WalkPathResult)) <= 0) {
                        printf("\033[31mSend failed (simulation result).\033[0m\n");
                        destroyWorld(&world);
                        ipcServerStop(&srv);
                        return 1;
                    }
                }

                printf("\nServer replied with simulation result.\n");

                FILE *fs = fopen(fReq.filename, "w");
                if (!fs) {
                    printf("\033[31mFailed to open file for saving.\033[0m\n");
                    destroyWorld(&world);
                    ipcServerStop(&srv);
                    return 1;
                }

                fprintf(fs, "mMode: %d\n", mapReq.obstaclesMode);
                fprintf(fs, "worldX: %d\n", mapReq.sizeX);
                fprintf(fs, "worldY: %d\n", mapReq.sizeY);
                for (int y = 0; y < world.sizeY; ++y) {
                    for (int x = 0; x < world.sizeX; ++x) {
                        fputc(WORLD_AT(&world, x, y), fs);
                        if (x < world.sizeX - 1) fputc(' ', fs);
                    }
                    fputc('\n', fs);
                }

                fprintf(fs, "prUp: %.6f\n", req.p_up);
                fprintf(fs, "prDown: %.6f\n", req.p_down);
                fprintf(fs, "prLeft: %.6f\n", req.p_left);
                fprintf(fs, "prRight: %.6f\n", req.p_right);
                fprintf(fs, "K: %d\n\n", req.maxSteps);

                for (int i = 1; i < (2 * mapReq.sizeX); i++) {
                    fprintf(fs, "-");
                }

                fprintf(fs, "\nSimulation Result:\n");

                if (modeReq.mode == 1) {
                    drawResultMap(fs, world.sizeX, world.sizeY, resRep, repReq.printMode);
                } else if (modeReq.mode == 2) {
                    drawPath(fs, resPath);
                }

                fclose(fs);
                printf("Simulation saved to '%s'.\n", fReq.filename);
                break;
            }
            case MSG_LOAD: {
                printf("File load request received.\n");

                FILE *fl = fopen(fReq.filename, "r");
                if (!fl) {
                    printf("\033[31mFailed to open file for loading.\033[0m\n");
                    destroyWorld(&world);
                    ipcServerStop(&srv);
                    return 1;
                }

                char line[1024];
                modeReq.mode = 1;

                if (!fgets(line, sizeof(line), fl) ||
                    sscanf(line, "mMode: %d", &mapReq.obstaclesMode) != 1) {
                    printf("Invalid mMode\n");
                    fclose(fl);
                    break;
                    }

                if (!fgets(line, sizeof(line), fl) ||
                    sscanf(line, "worldX: %d", &mapReq.sizeX) != 1) {
                    printf("Invalid worldX\n");
                    fclose(fl);
                    break;
                    }

                if (!fgets(line, sizeof(line), fl) ||
                    sscanf(line, "worldY: %d", &mapReq.sizeY) != 1) {
                    printf("Invalid worldY\n");
                    fclose(fl);
                    break;
                    }

                destroyWorld(&world);
                world = createWorld(mapReq.sizeX, mapReq.sizeY);

                WorldRequest wReq;
                for (int y = 0; y < world.sizeY; ++y) {
                    if (!fgets(line, sizeof(line), fl)) {
                        printf("Invalid world row\n");
                        fclose(fl);
                        break;
                    }

                    const char *tok = strtok(line, " \n");
                    for (int x = 0; x < world.sizeX && tok; ++x) {
                        const char wItem = tok[0];

                        WORLD_AT(&world, x, y) = wItem;
                        wReq.world[x][y] = wItem;

                        tok = strtok(NULL, " \n");
                    }
                }

                if (!fgets(line, sizeof(line), fl) ||
                    sscanf(line, "prUp: %lf", &req.p_up) != 1 ||
                    !fgets(line, sizeof(line), fl) ||
                    sscanf(line, "prDown: %lf", &req.p_down) != 1 ||
                    !fgets(line, sizeof(line), fl) ||
                    sscanf(line, "prLeft: %lf", &req.p_left) != 1 ||
                    !fgets(line, sizeof(line), fl) ||
                    sscanf(line, "prRight: %lf", &req.p_right) != 1) {
                    printf("Invalid probabilities\n");
                    fclose(fl);
                    break;
                    }

                if (!fgets(line, sizeof(line), fl) ||
                    sscanf(line, "K: %d", &req.maxSteps) != 1) {
                    printf("Invalid K\n");
                    fclose(fl);
                    break;
                    }

                fclose(fl);

                printf("Simulation loaded successfully from '%s'.\n", fReq.filename);
                load = 1;

                LoadedResponse lRes;
                lRes.mapReq = mapReq;
                lRes.wReq = wReq;
                lRes.sReq = req;

                if (ipcServerSend(&srv, (char *) &lRes, sizeof(LoadedResponse)) <= 0) {
                    printf("\033[31mSend failed (loading result).\033[0m\n");
                    destroyWorld(&world);
                    ipcServerStop(&srv);
                    return 1;
                }

                printf("Server replied with loading result.\n");
                break;
            }
            case MSG_FILE: {
                printf("\nWaiting for client file name request...\n");

                const int frs = ipcServerRecv(&srv, (char *) &fReq, sizeof(FileRequest));

                if (frs <= 0) {
                    printf("\033[31mReceive failed (file name).\033[0m\n");
                    destroyWorld(&world);
                    ipcServerStop(&srv);
                    return 1;
                }

                printf("File name request received.\n");
                break;
            }
        }
    }
}
