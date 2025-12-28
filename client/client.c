#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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

    if (value <= 0) {
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
        drawWorldWithWalker(
            result,
            result.path[i].x,
            result.path[i].y
        );
        printf("\n");

        usleep(200000); // 200 ms
    }

    if (result.success) {
        printf("Walker successfully walked to (0, 0).\n");
    }
}

void drawResultMap(const int sizeX, const int sizeY, const WalkResults results[sizeY][sizeX], const int printMode) { // VLA
    double highestAvgStepCount = 0;

    for (int y = 0; y < sizeY; ++y) {
        for (int x = 0; x < sizeX; ++x) {
            if (results[y][x].avgStepCount > highestAvgStepCount) {
                highestAvgStepCount = results[y][x].avgStepCount;
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
            } else if (results[y][x].avgStepCount == 0) {
                printf("  #  ");
            } else {
                if (printMode == 1) {
                    printf("%.2f ", results[y][x].probSuccess);
                } else if (printMode == 2) {
                    printf("%.2f", results[y][x].avgStepCount);

                    if (results[y][x].avgStepCount > 0) {
                        currCellWidth = (int)log10(abs((int)results[y][x].avgStepCount)) + 1;
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
        int successful = 0;
        int x, y, K, replications, sizeX, sizeY, mode, printMode;
        double up, down, left, right;

        while (!successful) {
            printf("\nPress Y to continue or N to quit. ");

            char ch;
            if (scanf("%c", &ch) != 1) {
                printf("Error while reading user input.\n");
                clearInput();
                successful = 0;
                continue;
            }

            if (ch == 'N' || ch == 'n') {
                SimRequest req;
                req.end = 1;

                if (ipc_client_send(&cli, (char*)&req, sizeof(SimRequest)) <= 0) {
                    printf("Send failed.\n");
                    ipc_client_close(&cli);
                    return 1;
                }

                printf("\nClient sent cancellation.\n");
                ipc_client_close(&cli);
                return 0;
            } else if (ch != 'Y' && ch != 'y') {
                printf("Incorrect user input.\n");
                clearInput();
                successful = 0;
                continue;
            }
            clearInput();

            if (!readInt("Choose simulation mode (1 -> summary, 2 -> interactive) ", &mode) ||
                (mode != 1 && mode != 2)) {
                printf("Invalid simulation mode selected.\n");
                successful = 0;
                continue;
            }

            if (mode == 1) {
                if (!readInt("Choose printing mode (1 -> probabilities, 2 -> steps count) ", &printMode) ||
                    (printMode != 1 && printMode != 2)) {
                    printf("Invalid printing mode selected.\n");
                    successful = 0;
                    continue;
                    }
            }

            printf("\nEnter world size...\n");
            if ((!readInt("Enter X length: ", &sizeX) ||
                !readInt("Enter Y length: ", &sizeY)) ||
                (sizeX < 2 || sizeY < 2)) {
                printf("Invalid size input, world must be at least 2x2.\n");
                successful = 0;
                continue;
            }
            if (mode == 2 && (sizeX > MAX_WORLD_X || sizeY > MAX_WORLD_Y)) {
                printf("Invalid size input, cannot be bigger than %dx%d.\n", MAX_WORLD_X, MAX_WORLD_Y);
                successful = 0;
                continue;
            }

            printf("\nWorld size set to %dx%d.\n", sizeX, sizeY);

            if (mode == 2) {
                printf("\nEnter starting position...\n");
                if (!readInt("Enter starting X: ", &x) ||
                    !readInt("Enter starting Y: ", &y)) {
                    printf("Invalid position input.\n");
                    successful = 0;
                    continue;
                    } else if (x >= sizeX || y >= sizeY) {
                        printf("Starting position can't be outside the world.\n");
                        successful = 0;
                        continue;
                    }

                printf("\nStarting position set to [%d, %d].\n", x, y);
            }

            printf("\nEnter direction probabilities...\n");
            if (!readDouble("Enter prob up: ", &up, 0, 4) ||
                !readDouble("Enter prob down: ", &down, up, 3) ||
                !readDouble("Enter prob left: ", &left, (up + down), 2) ||
                !readDouble("Enter prob right: ", &right, (up + down + left), 1)) {
                printf("Invalid probability input.\n");
                successful = 0;
                continue;
            }

            const double sum = fabs(up + down + left + right);
            if (sum < 0.999 || sum > 1.001) { // lebo sucet double nikdy nie je presne 1
                printf("Probabilities must be >= 0 and sum to 1.\n");
                successful = 0;
                continue;
            }
            printf("\nDirection probabilities set to {up: %lf, down: %lf, left: %lf, right: %lf}.\n\n", up, down, left, right);

            if (!readInt("Enter max steps K: ", &K)) {
                printf("K must be > 0.\n");
                successful = 0;
                continue;
            }
            if (mode == 2 && K > MAX_PATH - 1) {
                printf("K must be <= %d.\n", MAX_PATH - 1);
                successful = 0;
                continue;
            }

            if (mode == 1) {
                if (!readInt("Enter replications count: ", &replications) || replications <= 0) {
                    printf("Replications must be > 0.\n");
                    successful = 0;
                    continue;
                }
            }

            successful = 1;
        }

        SimRequest req;
        req.mode = mode;
        req.startX = x;
        req.startY = y;
        req.sizeX = sizeX;
        req.sizeY = sizeY;
        req.p_up = up;
        req.p_down = down;
        req.p_left = left;
        req.p_right = right;
        req.maxSteps = K;
        req.replications = replications;

        if (ipc_client_send(&cli, (char*)&req, sizeof(SimRequest)) <= 0) {
            printf("Send failed.\n");
            ipc_client_close(&cli);
            return 1;
        }
        printf("\nClient sent simulation request.\n");

        if (mode == 1) {
            WalkResults res[sizeY][sizeX];

            const int r = ipc_client_recv(&cli, (char*)&res, sizeX * sizeY * sizeof(WalkResults));
            if (r <= 0) {
                printf("Receive failed.\n");
                ipc_client_close(&cli);
                return 1;
            }

            printf("\nClient received simulation result.\n");

            drawResultMap(sizeX, sizeY, res, printMode);
        } else if (mode == 2) {
            WalkPathResult res;

            const int r = ipc_client_recv(&cli, (char*)&res, sizeof(WalkPathResult));
            if (r <= 0) {
                printf("Receive failed.\n");
                ipc_client_close(&cli);
                return 1;
            }

            drawPath(res);
        }
    }
}