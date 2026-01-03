#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "../common/ipc.h"
#include "../common/protocol.h"
#include "../common/randomWalk.h"

#define INPUT_BUFFER 64

int readInt(const char *prompt, int *out) {
    char input[INPUT_BUFFER];
    printf("%s", prompt);

    if (!fgets(input, sizeof(input), stdin)) {
        return 0; // input error
    }

    if ((input[0] == 'N' || input[0] == 'n') && input[1] == '\n') {
        return -1;
    }

    const long value = strtol(input, NULL, 10);
    if (value < 0) {
        return 0;
    }

    *out = (int) value;
    return 1;
}

int readDouble(const char *prompt, double *out, const double probSum, const int divider) {
    printf("%s", prompt);
    char input[INPUT_BUFFER];

    if (!fgets(input, INPUT_BUFFER, stdin)) {
        return 0; // input error
    }

    if ((input[0] == 'N' || input[0] == 'n') && input[1] == '\n') {
        return -1;
    }

    if (strlen(input) > 0 && input[strlen(input) - 1] == '\n') {
        input[strlen(input) - 1] = '\0';
    }

    if (strchr(input, '*') != NULL) {
        *out = (1 - probSum) / divider;
        return 1;
    } else {
        char *endPtr;
        const double value = strtod(input, &endPtr);

        if (endPtr == input || value < 0) {
            return 0;
        } else {
            *out = value;
            return 1;
        }
    }
}

void drawWorldWithWalker(const WalkPathResult result, const int walkerX, const int walkerY) {
    for (int y = 0; y < result.worldY; ++y) {
        for (int x = 0; x < result.worldX; ++x) {
            if (x == walkerX && y == walkerY) {
                putchar('@'); // walker
            } else {
                putchar(result.world[x][y]);
            }
        }
        putchar('\n');
    }
}

void drawPath(const WalkPathResult result) {
    for (int i = 0; i < result.pathLen; ++i) {
        printf("Step %d / %d : (%d, %d)\n", i, result.pathLen - 1, result.path[i].x, result.path[i].y);

        if (i > 0) {
            char where[28];

            if (result.path[i - 1].x < result.path[i].x) {
                if (result.path[i].x - result.path[i - 1].x > 1) {
                    // presiel za okraj
                    strcpy(where, "'left' (crossed the world)");
                } else {
                    strcpy(where, "'right'");
                }
            } else if (result.path[i - 1].x > result.path[i].x) {
                if (result.path[i - 1].x - result.path[i].x > 1) {
                    // presiel za okraj
                    strcpy(where, "'right' (crossed the world)");
                } else {
                    strcpy(where, "'left'");
                }
            } else if (result.path[i - 1].y < result.path[i].y) {
                if (result.path[i].y - result.path[i - 1].y > 1) {
                    // presiel za okraj
                    strcpy(where, "'up' (crossed the world)");
                } else {
                    strcpy(where, "'down'");
                }
            } else if (result.path[i - 1].y > result.path[i].y) {
                if (result.path[i - 1].y - result.path[i].y > 1) {
                    // presiel za okraj
                    strcpy(where, "'down' (crossed the world)");
                } else {
                    strcpy(where, "'up'");
                }
            }

            printf("Walker moved %s.\n", where);
        }

        drawWorldWithWalker(result, result.path[i].x, result.path[i].y);
        printf("\n");

        usleep(200000); // 200 ms
    }

    if (result.success) {
        printf("Walker successfully walked to (0, 0).\n\n");
    } else if (result.stuck) {
        printf("Walker got stuck.\n\n");
    }
}

// VLA
void drawResultMap(const int sizeX, const int sizeY, const WalkResult results[sizeX][sizeY], const int printMode) {
    double highestAvgStepCount = 0;

    for (int y = 0; y < sizeY; ++y) {
        for (int x = 0; x < sizeX; ++x) {
            if (results[x][y].avgStepCount > highestAvgStepCount) {
                highestAvgStepCount = results[x][y].avgStepCount;
            }
        }
    }

    int mostDigits = 1;
    if (printMode == 2) {
        if ((int) highestAvgStepCount > 0) {
            mostDigits = (int) log10(abs((int) highestAvgStepCount)) + 1;
        }

        if (mostDigits < 2) {
            mostDigits = 2;
        }

        mostDigits += 3; // lebo 2 desatinne miesta + bodka
    }

    for (int y = 0; y < sizeY; ++y) {
        for (int x = 0; x < sizeX; ++x) {
            int currCellWidth = 5; // default v pripade W, # alebo -1.00

            if (x == 0 && y == 0) {
                printf("  W  ");
            } else if (results[x][y].avgStepCount == 0) {
                printf("  #  ");
            } else {
                if (printMode == 1) {
                    printf("%.2f ", results[x][y].probSuccess);
                } else if (printMode == 2) {
                    printf("%.2f", results[x][y].avgStepCount);

                    if (results[x][y].avgStepCount > 0) {
                        currCellWidth = (int) log10(abs((int) results[x][y].avgStepCount)) + 1;
                        currCellWidth += 3;
                    }
                }
            }

            if (printMode == 2) {
                printf("%*s", (mostDigits - currCellWidth) + 1, ""); // +1 pre medzeru medzi vypismi
            }
        }
        printf("\n");
    }

    printf("Highest average step count: %.4f\n\n", highestAvgStepCount);
}

int clientExit(ipc_client cli) {
    MessageHeader h;
    h.type = MSG_EXIT;

    if (ipc_client_send(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
        printf("\033[31mSend failed (header).\033[0m\n");
        ipc_client_close(&cli);
        return 1;
    }

    printf("\nClient sent exit request.\n");
    ipc_client_close(&cli);
    return 0;
}

int main(void) {
    ipc_client cli;

    printf(
        "██████╗░░█████╗░███╗░░██╗██████╗░░█████╗░███╗░░░███╗░░░██╗░░░░░░░██╗░█████╗░██╗░░░░░██╗░░██╗░░░█████╗░░██████╗\n"
        "██╔══██╗██╔══██╗████╗░██║██╔══██╗██╔══██╗████╗░████║░░░██║░░██╗░░██║██╔══██╗██║░░░░░██║░██╔╝░░██╔══██╗██╔════╝\n"
        "██████╔╝███████║██╔██╗██║██║░░██║██║░░██║██╔████╔██║░░░╚██╗████╗██╔╝███████║██║░░░░░█████═╝░░░██║░░██║╚█████╗░\n"
        "██╔══██╗██╔══██║██║╚████║██║░░██║██║░░██║██║╚██╔╝██║░░░░████╔═████║░██╔══██║██║░░░░░██╔═██╗░░░██║░░██║░╚═══██╗\n"
        "██║░░██║██║░░██║██║░╚███║██████╔╝╚█████╔╝██║░╚═╝░██║░░░░╚██╔╝░╚██╔╝░██║░░██║███████╗██║░╚██╗░░╚█████╔╝██████╔╝\n"
        "╚═╝░░╚═╝╚═╝░░╚═╝╚═╝░░╚══╝╚═════╝░░╚════╝░╚═╝░░░░░╚═╝░░░░░╚═╝░░░╚═╝░░╚═╝░░╚═╝╚══════╝╚═╝░░╚═╝░░░╚════╝░╚═════╝░\n"
    );
    printf("\033[90mYou can press N to quit at any time...\033[0m\n\n");

    printf(
        "┏━┳━┓\n"
        "┃┃┃┃┣━┳━┳┳┳┳┓\n"
        "┃┃┃┃┃┻┫┃┃┃┃┣┫\n"
        "┗┻━┻┻━┻┻━┻━┻┛\n"
    );

    printf(
        "New Simulation  [1]\n"
        "Join Simulation [2]\n"
        "Load Simulation [3]\n"
        "End Program     [4]\n\n"
    );

    int programMode;

    while (1) {
        const int res = readInt(">", &programMode);
        if (res == -1 || programMode == 4) {
            printf("Ending program.");
            return 0;
        }

        if (!res || (programMode != 1 && programMode != 2 && programMode != 3 && programMode != 4)) {
            printf("\033[31mInvalid menu option.\033[0m\n");
            continue;
        }

        break;
    }

    int x, y, K, replications, sizeX, sizeY, mode, printMode, obstaclesMode;
    double up, down, left, right;
    MessageHeader h;
    pid_t pid;

    mode = 1; // sumarny mod je default

    switch (programMode) {
        case 1:
            pid = fork();
            if (pid == 0) {
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                execl("./server", "./server", IPC_PORT, NULL);
                perror("Failed to launch server");
                exit(1);
            } else if (pid > 0) {
                printf("Server started in background.\n");
                sleep(2); // cakam na spustenie servera
                if (ipc_client_connect(&cli, IPC_PORT) != 0) {
                    printf("\033[31mConnection failed.\033[0m\n");
                    return 1;
                }
                printf("Client connected to port %s.\n\n", IPC_PORT);

                while (1) {
                    while (1) {
                        const int res = readInt("Choose simulation mode (1 -> summary, 2 -> interactive) ", &mode);
                        if (res == -1) {
                            return clientExit(cli);
                        }

                        if (!res || (mode != 1 && mode != 2)) {
                            printf("\033[31mInvalid simulation mode selected.\033[0m\n");
                            continue;
                        }
                        break;
                    }

                    h.type = MSG_MODE;
                    if (ipc_client_send(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
                        printf("\033[31mSend failed (header).\033[0m\n");
                        ipc_client_close(&cli);
                        return 1;
                    }

                    ModeRequest modeReq;
                    modeReq.mode = mode;
                    if (ipc_client_send(&cli, (char *) &modeReq, sizeof(ModeRequest)) <= 0) {
                        printf("\033[31mSend failed (mode).\033[0m\n");
                        ipc_client_close(&cli);
                        return 1;
                    }
                    printf("\nClient sent mode request.\n\n");

                    if (mode == 1) {
                        while (1) {
                            const int res = readInt("Choose printing mode (1 -> probabilities, 2 -> steps count) ",
                                                    &printMode);
                            if (res == -1) {
                                return clientExit(cli);
                            }

                            if (!res || (printMode != 1 && printMode != 2)) {
                                printf("\033[31mInvalid printing mode selected.\033[0m\n");
                                continue;
                            }
                            break;
                        }
                    }

                    while (1) {
                        const int res = readInt("Choose map mode (1 -> with obstacles, 2 -> no obstacles) ",
                                                &obstaclesMode);
                        if (res == -1) {
                            return clientExit(cli);
                        }

                        if (!res || (obstaclesMode != 1 && obstaclesMode != 2)) {
                            printf("\033[31mInvalid map mode selected.\033[0m\n");
                            continue;
                        }
                        break;
                    }

                    while (1) {
                        printf("\nEnter world size...\n");
                        const int res1 = readInt("Enter X length: ", &sizeX);
                        if (res1 == -1) {
                            return clientExit(cli);
                        }
                        const int res2 = readInt("Enter Y length: ", &sizeY);
                        if (res2 == -1) {
                            return clientExit(cli);
                        }

                        if ((!res1 || !res2) || (sizeX < 2 || sizeY < 2)) {
                            printf("\033[31mInvalid size input, world must be at least 2x2.\033[0m\n");
                            continue;
                        }
                        if (mode == 2 && (sizeX > MAX_WORLD_X || sizeY > MAX_WORLD_Y)) {
                            printf("\033[31mInvalid size input, cannot be bigger than %dx%d.\033[0m\n", MAX_WORLD_X,
                                   MAX_WORLD_Y);
                            continue;
                        }
                        break;
                    }

                    h.type = MSG_MAP;
                    if (ipc_client_send(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
                        printf("\033[31mSend failed (header).\033[0m\n");
                        ipc_client_close(&cli);
                        return 1;
                    }

                    MapRequest mapReq;
                    mapReq.obstaclesMode = obstaclesMode;
                    mapReq.sizeX = sizeX;
                    mapReq.sizeY = sizeY;

                    if (ipc_client_send(&cli, (char *) &mapReq, sizeof(MapRequest)) <= 0) {
                        printf("\033[31mSend failed (map).\033[0m\n");
                        ipc_client_close(&cli);
                        return 1;
                    }
                    printf("\nClient sent map request.\n");

                    WorldRequest wRes;
                    const int mr = ipc_client_recv(&cli, (char *) &wRes, sizeof(WorldRequest));
                    if (mr <= 0) {
                        printf("\033[31mReceive failed (world).\033[0m\n");
                        ipc_client_close(&cli);
                        return 1;
                    }

                    printf("World size set to %dx%d.\n", sizeX, sizeY);

                    for (int j = 0; j < sizeY; j++) {
                        for (int k = 0; k < sizeX; k++) {
                            printf("%c ", wRes.world[k][j]);
                        }
                        printf("\n");
                    }

                    if (mode == 2) {
                        while (1) {
                            printf("\nEnter starting position...\n");
                            const int res1 = readInt("Enter starting X: ", &x);
                            if (res1 == -1) {
                                return clientExit(cli);
                            }
                            const int res2 = readInt("Enter starting Y: ", &y);
                            if (res2 == -1) {
                                return clientExit(cli);
                            }

                            if (!res1 || !res2) {
                                printf("\033[31mInvalid position input.\033[0m\n");
                                continue;
                            }
                            if (x == 0 && y == 0) {
                                printf("\033[31mStarting position cannot be [0, 0].\033[0m\n");
                                continue;
                            }
                            if (x >= sizeX || y >= sizeY) {
                                printf("\033[31mStarting position cannot be outside the world.\033[0m\n");
                                continue;
                            }

                            h.type = MSG_START_POS;
                            if (ipc_client_send(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
                                printf("\033[31mSend failed (header).\033[0m\n");
                                ipc_client_close(&cli);
                                return 1;
                            }

                            StartPositionRequest startReq;
                            startReq.startX = x;
                            startReq.startY = y;

                            if (ipc_client_send(&cli, (char *) &startReq, sizeof(StartPositionRequest)) <= 0) {
                                printf("\033[31mSend failed (starting position).\033[0m\n");
                                ipc_client_close(&cli);
                                return 1;
                            }
                            printf("\nClient sent starting position request.\n");

                            StartPositionResult st;
                            const int r = ipc_client_recv(&cli, (char *) &st,
                                                          sizeX * sizeY * sizeof(StartPositionResult));
                            if (r <= 0) {
                                printf("\033[31mReceive failed (starting position result).\033[0m\n");
                                ipc_client_close(&cli);
                                return 1;
                            }

                            printf("Client received starting position result.\n");

                            if (st.notOk) {
                                printf("\033[31mIncorrect starting position.\033[0m\n");
                            } else {
                                printf("Starting position set to [%d, %d].\n", x, y);
                                break;
                            }
                        }
                    }

                    while (1) {
                        printf("\nEnter direction probabilities...\n");
                        const int res1 = readDouble("Enter prob up: ", &up, 0, 4);
                        if (res1 == -1) {
                            return clientExit(cli);
                        }
                        const int res2 = readDouble("Enter prob down: ", &down, up, 3);
                        if (res2 == -1) {
                            return clientExit(cli);
                        }
                        const int res3 = readDouble("Enter prob left: ", &left, (up + down), 2);
                        if (res3 == -1) {
                            return clientExit(cli);
                        }
                        const int res4 = readDouble("Enter prob right: ", &right, (up + down + left), 1);
                        if (res4 == -1) {
                            return clientExit(cli);
                        }

                        if (!res1 || !res2 || !res3 || !res4) {
                            printf("\033[31mInvalid probability input.\033[0m\n");
                            continue;
                        }

                        const double sum = fabs(up + down + left + right);
                        if (sum < 0.999 || sum > 1.001) {
                            // lebo sucet double nikdy nie je presne 1
                            printf("\033[31mProbabilities must be >= 0 and sum to 1.\033[0m\n");
                            continue;
                        }
                        break;
                    }
                    printf("\nDirection probabilities set to {up: %lf, down: %lf, left: %lf, right: %lf}.\n\n", up,
                           down, left,
                           right);

                    while (1) {
                        const int res = readInt("Enter max steps K: ", &K);
                        if (res == -1) {
                            return clientExit(cli);
                        }

                        if (!res || K <= 0) {
                            printf("\033[31mK must be > 0.\033[0m\n");
                            continue;
                        }
                        if (mode == 2 && K > MAX_PATH - 1) {
                            printf("\033[31mK must be <= %d.\033[0m\n", MAX_PATH - 1);
                            continue;
                        }
                        break;
                    }
                    break;
                }
            } else {
                perror("fork failed");
                return 1;
            }
            break;

        case 2:
            if (ipc_client_connect(&cli, IPC_PORT) != 0) {
                printf("\033[31mConnection failed.\033[0m\n");
                return 1;
            }
            printf("Client connected to port %s.\n\n", IPC_PORT);
            break;

        case 3:
            printf("load simulation");
            break;

        case 4:
            printf("Ending program.\n");
            return 0;

        default:
            break;
    }

    if (mode == 1) {
        while (1) {
            const int res = readInt("Enter replications count: ", &replications);
            if (res == -1) {
                return clientExit(cli);
            }

            if (!res || replications <= 0) {
                printf("\033[31mReplications must be > 0.\033[0m\n");
                continue;
            }
            break;
        }
    }

    h.type = MSG_SIMULATION;
    if (ipc_client_send(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
        printf("\033[31mSend failed (header).\033[0m\n");
        ipc_client_close(&cli);
        return 1;
    }

    SimRequest req;
    req.p_up = up;
    req.p_down = down;
    req.p_left = left;
    req.p_right = right;
    req.maxSteps = K;
    req.replications = replications;

    if (ipc_client_send(&cli, (char *) &req, sizeof(SimRequest)) <= 0) {
        printf("\033[31mSend failed (simulation).\033[0m\n");
        ipc_client_close(&cli);
        return 1;
    }
    printf("\nClient sent simulation request.\n");

    if (mode == 1) {
        WalkResult res[sizeX][sizeY];

        const int r = ipc_client_recv(&cli, (char *) &res, sizeX * sizeY * sizeof(WalkResult));
        if (r <= 0) {
            printf("\033[31mReceive failed (simulation result).\033[0m\n");
            ipc_client_close(&cli);
            return 1;
        }

        printf("\nClient received simulation result.\n");

        drawResultMap(sizeX, sizeY, res, printMode);
    } else if (mode == 2) {
        WalkPathResult res;

        const int r = ipc_client_recv(&cli, (char *) &res, sizeof(WalkPathResult));
        if (r <= 0) {
            printf("\033[31mReceive failed (simulation result).\033[0m\n");
            ipc_client_close(&cli);
            return 1;
        }

        drawPath(res);
    }

    clientExit(cli);
}
