#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "../common/ipc.h"
#include "../common/protocol.h"
#include "../common/randomWalk.h"

#define INPUT_BUFFER 64

// tento kód mám od AI
void clearInput(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int readInt(const char* prompt, int* out) {
    char input[INPUT_BUFFER];
    printf("%s", prompt);

    if (!fgets(input, sizeof(input), stdin)) {
        return 0; // input error
    }

    const long value = strtol(input, NULL, 10);
    if (value < 0) {
        return 0;
    }

    *out = (int)value;
    return 1;
}

int readDouble(const char* prompt, double* out, const double probSum, const int divider) {
    printf("%s", prompt);
    char input[INPUT_BUFFER];

    if (!fgets(input, INPUT_BUFFER, stdin)) {
        return 0; // input error
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
                if (result.path[i].x - result.path[i - 1].x > 1) { // presiel za okraj
                    strcpy(where, "'left' (crossed the world)");
                } else {
                    strcpy(where, "'right'");
                }
            } else if (result.path[i - 1].x > result.path[i].x) {
                if (result.path[i - 1].x - result.path[i].x > 1) { // presiel za okraj
                    strcpy(where, "'right' (crossed the world)");
                } else {
                    strcpy(where, "'left'");
                }
            } else if (result.path[i - 1].y < result.path[i].y) {
                if (result.path[i].y - result.path[i - 1].y > 1) { // presiel za okraj
                    strcpy(where, "'up' (crossed the world)");
                } else {
                    strcpy(where, "'down'");
                }
            } else if (result.path[i - 1].y > result.path[i].y) {
                if (result.path[i - 1].y - result.path[i].y > 1) { // presiel za okraj
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
        printf("Walker successfully walked to (0, 0).\n");
    }
}

void drawResultMap(const int sizeX, const int sizeY, const WalkResult results[sizeX][sizeY], const int printMode) { // VLA
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
        if ((int)highestAvgStepCount > 0) {
            mostDigits = (int)log10(abs((int)highestAvgStepCount)) + 1;
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
                        currCellWidth = (int)log10(abs((int)results[x][y].avgStepCount)) + 1;
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

    printf("Highest average step count: %.4f\n", highestAvgStepCount);
}

int main(void) {
    ipc_client cli;
    if (ipc_client_connect(&cli, IPC_PORT) != 0) {
        printf("Connection failed.\n");
        return 1;
    }
    printf("Client connected to port %s.\n", IPC_PORT);

    while (1) {
        int x, y, K, replications, sizeX, sizeY, mode, printMode, obstaclesMode;
        double up, down, left, right;

        while (1) {
            printf("\033[31mThis text is red!\033[0m\n");   // 31 = red
            printf("\nPress Y to continue or N to quit. ");

            char ch;
            if (scanf("%c", &ch) != 1) {
                printf("Error while reading user input.\n");
                clearInput();
                continue;
            }

            if (ch == 'N' || ch == 'n') {
                EndRequest req;
                req.end = 1;

                if (ipc_client_send(&cli, (char*)&req, sizeof(EndRequest)) <= 0) {
                    printf("Send failed (exit).\n");
                    ipc_client_close(&cli);
                    return 1;
                }

                printf("\nClient sent cancellation.\n");
                ipc_client_close(&cli);
                return 0;
            } else if (ch != 'Y' && ch != 'y') {
                printf("Incorrect user input.\n");
                clearInput();
                continue;
            }
            EndRequest req;
            req.end = 0;

            if (ipc_client_send(&cli, (char*)&req, sizeof(EndRequest)) <= 0) {
                printf("Send failed (exit).\n");
                ipc_client_close(&cli);
                return 1;
            }
            clearInput();

            if (!readInt("Choose simulation mode (1 -> summary, 2 -> interactive) ", &mode) ||
                (mode != 1 && mode != 2)) {
                printf("Invalid simulation mode selected.\n");
                continue;
            }

            ModeRequest modeReq;
            modeReq.mode = mode;
            if (ipc_client_send(&cli, (char*)&modeReq, sizeof(ModeRequest)) <= 0) {
                printf("Send failed (mode).\n");
                ipc_client_close(&cli);
                return 1;
            }
            printf("\nClient sent mode request.\n\n");

            if (mode == 1) {
                if (!readInt("Choose printing mode (1 -> probabilities, 2 -> steps count) ", &printMode) ||
                    (printMode != 1 && printMode != 2)) {
                    printf("Invalid printing mode selected.\n");
                    continue;
                }
            }

            if (!readInt("Choose map mode (1 -> with obstacles, 2 -> no obstacles) ", &obstaclesMode) ||
                (obstaclesMode != 1 && obstaclesMode != 2)) {
                printf("Invalid map mode selected.\n");
                continue;
            }

            printf("\nEnter world size...\n");
            if ((!readInt("Enter X length: ", &sizeX) ||
                !readInt("Enter Y length: ", &sizeY)) ||
                (sizeX < 2 || sizeY < 2)) {
                printf("Invalid size input, world must be at least 2x2.\n");
                continue;
            }
            if (mode == 2 && (sizeX > MAX_WORLD_X || sizeY > MAX_WORLD_Y)) {
                printf("Invalid size input, cannot be bigger than %dx%d.\n", MAX_WORLD_X, MAX_WORLD_Y);
                continue;
            }

            MapRequest mapReq;
            mapReq.obstaclesMode = obstaclesMode;
            mapReq.sizeX = sizeX;
            mapReq.sizeY = sizeY;

            if (ipc_client_send(&cli, (char*)&mapReq, sizeof(MapRequest)) <= 0) {
                printf("Send failed (map).\n");
                ipc_client_close(&cli);
                return 1;
            }
            printf("\nClient sent map request.\n");
            printf("World size set to %dx%d.\n", sizeX, sizeY);

            if (mode == 2) {
                while (1) {
                    printf("\nEnter starting position...\n");
                    if (!readInt("Enter starting X: ", &x) ||
                        !readInt("Enter starting Y: ", &y)) {
                        printf("Invalid position input.\n");
                        continue;
                    }
                    if (x == 0 && y == 0) {
                        printf("Starting position can't be [0, 0].\n");
                        continue;
                    }
                    if (x >= sizeX || y >= sizeY) {
                        printf("Starting position can't be outside the world.\n");
                        continue;
                    }

                    StartPositionRequest startReq;
                    startReq.startX = x;
                    startReq.startY = y;

                    if (ipc_client_send(&cli, (char*)&startReq, sizeof(StartPositionRequest)) <= 0) {
                        printf("Send failed (starting position).\n");
                        ipc_client_close(&cli);
                        return 1;
                    }
                    printf("\nClient sent starting position request.\n");

                    StartPositionResult st;
                    const int r = ipc_client_recv(&cli, (char*)&st, sizeX * sizeY * sizeof(StartPositionResult));
                    if (r <= 0) {
                        printf("Receive failed (starting position result).\n");
                        ipc_client_close(&cli);
                        return 1;
                    }

                    printf("Client received starting position result.\n");

                    if (st.notOk) {
                        printf("Incorrect starting position.\n");
                    } else {
                        printf("Starting position set to [%d, %d].\n", x, y);
                        break;
                    }
                }
            }

            printf("\nEnter direction probabilities...\n");
            if (!readDouble("Enter prob up: ", &up, 0, 4) ||
                !readDouble("Enter prob down: ", &down, up, 3) ||
                !readDouble("Enter prob left: ", &left, (up + down), 2) ||
                !readDouble("Enter prob right: ", &right, (up + down + left), 1)) {
                printf("Invalid probability input.\n");
                continue;
            }

            const double sum = fabs(up + down + left + right);
            if (sum < 0.999 || sum > 1.001) { // lebo sucet double nikdy nie je presne 1
                printf("Probabilities must be >= 0 and sum to 1.\n");
                continue;
            }
            printf("\nDirection probabilities set to {up: %lf, down: %lf, left: %lf, right: %lf}.\n\n", up, down, left, right);

            if (!readInt("Enter max steps K: ", &K)) {
                printf("K must be >= 0.\n");
                continue;
            }
            if (mode == 2 && K > MAX_PATH - 1) {
                printf("K must be <= %d.\n", MAX_PATH - 1);
                continue;
            }

            if (mode == 1) {
                if (!readInt("Enter replications count: ", &replications) || replications <= 0) {
                    printf("Replications must be > 0.\n");
                    continue;
                }
            }

            break;
        }

        SimRequest req;
        req.p_up = up;
        req.p_down = down;
        req.p_left = left;
        req.p_right = right;
        req.maxSteps = K;
        req.replications = replications;

        if (ipc_client_send(&cli, (char*)&req, sizeof(SimRequest)) <= 0) {
            printf("Send failed (simulation).\n");
            ipc_client_close(&cli);
            return 1;
        }
        printf("\nClient sent simulation request.\n");

        if (mode == 1) {
            WalkResult res[sizeX][sizeY];

            const int r = ipc_client_recv(&cli, (char*)&res, sizeX * sizeY * sizeof(WalkResult));
            if (r <= 0) {
                printf("Receive failed (simulation result).\n");
                ipc_client_close(&cli);
                return 1;
            }

            printf("\nClient received simulation result.\n");

            drawResultMap(sizeX, sizeY, res, printMode);
        } else if (mode == 2) {
            WalkPathResult res;

            const int r = ipc_client_recv(&cli, (char*)&res, sizeof(WalkPathResult));
            if (r <= 0) {
                printf("Receive failed (simulation result).\n");
                ipc_client_close(&cli);
                return 1;
            }

            drawPath(res);
        }
    }
}