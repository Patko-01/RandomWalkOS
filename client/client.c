#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "../common/ipc.h"
#include "../common/printer.h"
#include "../common/protocol.h"

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

int clientExit(ipcClient cli) {
    MessageHeader h = {0};
    h.type = MSG_EXIT;

    if (ipcClientSend(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
        printf("\033[31mSend failed (header).\033[0m\n");
        ipcClientClose(&cli);
        return 1;
    }

    printf("Client sent exit request.\n");
    ipcClientClose(&cli);
    return 0;
}

int setFileName(ipcClient cli) {
    while (1) {
        FileRequest fReq = {0};

        printf("Enter filename: ");
        fflush(stdout);

        if (!fgets(fReq.filename, sizeof(fReq.filename), stdin)) {
            printf("\033[31mInput error.\033[0m\n");
            continue;
        }

        if (strlen(fReq.filename) == 0) {
            printf("\033[31Invalid input size.\033[0m\n");
            clientExit(cli);
            continue;
        }

        fReq.filename[strlen(fReq.filename) - 1] = '\0';

        if (fReq.filename[0] == 'n' || fReq.filename[0] == 'N') {
            clientExit(cli);
            return 0;
        }

        MessageHeader h = {0};
        h.type = MSG_FILE;
        if (ipcClientSend(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
            printf("\033[31mSend failed (header).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }

        if (ipcClientSend(&cli, (char *) &fReq, sizeof(FileRequest)) <= 0) {
            printf("\033[31mSend failed (file).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }
        break;
    }

    return 0;
}

int readModeFromUser(ipcClient cli, int *mode) {
    while (1) {
        const int res = readInt("Choose simulation mode (1 -> summary, 2 -> interactive) ", mode);
        if (res == -1) {
            return 1;
        }

        if (!res || (*mode != 1 && *mode != 2)) {
            printf("\033[31mInvalid simulation mode selected.\033[0m\n");
            continue;
        }
        break;
    }

    MessageHeader h = {0};
    h.type = MSG_MODE;
    if (ipcClientSend(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
        printf("\033[31mSend failed (header).\033[0m\n");
        ipcClientClose(&cli);
        return 1;
    }

    ModeRequest modeReq = {0};
    modeReq.mode = *mode;
    if (ipcClientSend(&cli, (char *) &modeReq, sizeof(ModeRequest)) <= 0) {
        printf("\033[31mSend failed (mode).\033[0m\n");
        ipcClientClose(&cli);
        return 1;
    }

    printf("\nClient sent mode request.\n\n");
    return 0;
}

int readFromUser(ipcClient cli, int *mode, int *sizeX, int *sizeY, int *obstaclesMode, int *K, double *up, double *down, double *left, double *right) {
    MessageHeader h = {0};

        if (readModeFromUser(cli, mode) != 0) {
            return 1;
        }

        while (1) {
            const int res = readInt("Choose map mode (1 -> with obstacles, 2 -> no obstacles) ", obstaclesMode);
            if (res == -1) {
                return 1;
            }

            if (!res || (*obstaclesMode != 1 && *obstaclesMode != 2)) {
                printf("\033[31mInvalid map mode selected.\033[0m\n");
                continue;
            }
            break;
        }

        while (1) {
            printf("\nEnter world size...\n");
            const int res1 = readInt("Enter X length: ", sizeX);
            if (res1 == -1) {
                return 1;
            }
            const int res2 = readInt("Enter Y length: ", sizeY);
            if (res2 == -1) {
                return 1;
            }

            if ((!res1 || !res2) || (*sizeX < 2 || *sizeY < 2)) {
                printf("\033[31mInvalid size input, world must be at least 2x2.\033[0m\n");
                continue;
            }
            if (*sizeX > MAX_WORLD_X || *sizeY > MAX_WORLD_Y) {
                printf("\033[31mInvalid size input, cannot be bigger than %dx%d.\033[0m\n", MAX_WORLD_X, MAX_WORLD_Y);
                continue;
            }
            break;
        }

        h.type = MSG_MAP;
        if (ipcClientSend(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
            printf("\033[31mSend failed (header).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }

        MapRequest mapReq = {0};
        mapReq.obstaclesMode = *obstaclesMode;
        mapReq.sizeX = *sizeX;
        mapReq.sizeY = *sizeY;

        if (ipcClientSend(&cli, (char *) &mapReq, sizeof(MapRequest)) <= 0) {
            printf("\033[31mSend failed (map).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }
        printf("\nClient sent map request.\n");

        WorldRequest wRes = {0};
        const int mr = ipcClientRecv(&cli, (char *) &wRes, sizeof(WorldRequest));
        if (mr <= 0) {
            printf("\033[31mReceive failed (world).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }

        printf("World size set to %dx%d.\n", *sizeX, *sizeY);

        for (int j = 0; j < *sizeY; j++) {
            for (int k = 0; k < *sizeX; k++) {
                printf("%c ", wRes.world[k][j]);
            }
            printf("\n");
        }

        while (1) {
            printf("\nEnter direction probabilities...\n");
            const int res1 = readDouble("Enter prob up: ", up, 0, 4);
            if (res1 == -1) {
                return 1;
            }
            const int res2 = readDouble("Enter prob down: ", down, *up, 3);
            if (res2 == -1) {
                return 1;
            }
            const int res3 = readDouble("Enter prob left: ", left, (*up + *down), 2);
            if (res3 == -1) {
                return 1;
            }
            const int res4 = readDouble("Enter prob right: ", right, (*up + *down + *left), 1);
            if (res4 == -1) {
                return 1;
            }

            if (!res1 || !res2 || !res3 || !res4) {
                printf("\033[31mInvalid probability input.\033[0m\n");
                continue;
            }

            const double sum = fabs(*up + *down + *left + *right);
            if (sum < 0.999 || sum > 1.001) {
                // lebo sucet double nikdy nie je presne 1
                printf("\033[31mProbabilities must be >= 0 and sum to 1.\033[0m\n");
                continue;
            }
            break;
        }
        printf("\nDirection probabilities set to {up: %lf, down: %lf, left: %lf, right: %lf}.\n\n", *up, *down, *left, *right);

        while (1) {
            const int res = readInt("Enter max steps K: ", K);
            if (res == -1) {
                return 1;
            }

            if (!res || *K <= 0) {
                printf("\033[31mK must be > 0.\033[0m\n");
                continue;
            }
            if (*mode == 2 && *K > MAX_PATH - 1) {
                printf("\033[31mK must be <= %d.\033[0m\n", MAX_PATH - 1);
                continue;
            }
            break;
        }
    return 0;
}

int main(void) {
    ipcClient cli;

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

        if (!res || (programMode != 1 && programMode != 2 && programMode != 3)) {
            printf("\033[31mInvalid menu option.\033[0m\n");
            continue;
        }

        break;
    }

    int x, y, K, replications, sizeX, sizeY, mode, printMode, obstaclesMode;
    double up, down, left, right;
    MessageHeader h = {0};
    pid_t pid;

    int load = 0;
    mode = 1; // sumarny mod je default

    if (programMode == 1) {
        pid = fork();
        if (pid == 0) {
            (void)freopen("/dev/null", "w", stdout);
            (void)freopen("/dev/null", "w", stderr);
            execl("./server", "./server", IPC_PORT, NULL);
            perror("Failed to launch server");
            exit(1);
        } else if (pid > 0) {
            printf("Server started in background.\n");
            sleep(2); // cakam na spustenie servera
            if (ipcClientConnect(&cli, IPC_PORT) != 0) {
                printf("\033[31mConnection failed.\033[0m\n");
                return 1;
            }
            printf("Client connected to port %s.\n\n", IPC_PORT);

        } else {
            perror("fork failed");
            return 1;
        }
    } else if (programMode == 2) {
        if (ipcClientConnect(&cli, IPC_PORT) != 0) {
            printf("\033[31mConnection failed.\033[0m\n");
            return 1;
        }
        printf("Client connected to port %s.\n\n", IPC_PORT);
    }

    if (programMode == 1 || programMode == 2) {
        if (setFileName(cli) > 0) {
            return clientExit(cli);
        }
        if (readFromUser(cli, &mode, &sizeX, &sizeY, &obstaclesMode, &K, &up, &down, &left, &right) != 0) {
            return clientExit(cli);
        }
    }

    if (programMode == 3) {
        pid = fork();
        if (pid == 0) {
            (void)freopen("/dev/null", "w", stdout);
            (void)freopen("/dev/null", "w", stderr);
            execl("./server", "./server", IPC_PORT, NULL);
            perror("Failed to launch server");
            exit(1);
        } else if (pid > 0) {
            printf("Server started in background.\n");
            sleep(2); // cakam na spustenie servera
        } else {
            perror("fork failed");
            return 1;
        }
        if (ipcClientConnect(&cli, IPC_PORT) != 0) {
            printf("\033[31mConnection failed.\033[0m\n");
            return 1;
        }
        printf("Client connected to port %s.\n\n", IPC_PORT);

        if (setFileName(cli) > 0) {
            return 1;
        }

        h.type = MSG_LOAD;
        if (ipcClientSend(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
            printf("\033[31mSend failed (header).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }
        load = 1;

        LoadedResponse lr = {0};
        const int r = ipcClientRecv(&cli, (char *) &lr, sizeof(LoadedResponse));
        if (r <= 0) {
            printf("\033[31mReceive failed (loading result).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }

        sizeX = lr.mapReq.sizeX;
        sizeY = lr.mapReq.sizeY;
        obstaclesMode = lr.mapReq.obstaclesMode;
        up = lr.sReq.p_up;
        down = lr.sReq.p_down;
        left = lr.sReq.p_left;
        right = lr.sReq.p_right;
        K = lr.sReq.maxSteps;

        printf("\n-------------------------------\n");
        printf("Client received loading result.\n");
        printf("Direction probabilities set to {up: %lf, down: %lf, left: %lf, right: %lf}.\n", up, down, left, right);
        printf("World size set to %dx%d.\n", sizeX, sizeY);

        for (int j = 1; j < sizeX * 2; ++j) {
            printf("-");
        }
        printf("\n");

        for (int j = 0; j < sizeY; j++) {
            for (int k = 0; k < sizeX; k++) {
                printf("%c ", lr.wReq.world[k][j]);
            }
            printf("\n");
        }

        for (int j = 1; j < sizeX * 2; ++j) {
            printf("-");
        }
        printf("\n");

        printf("K = %d\n\n", K);

        while (1) {
            char prompt[150];
            char currMode[15];

            strcpy(currMode, mode == 1 ? "summary" : "interactive");
            sprintf(prompt, "Do you wish to change simulation mode? (1 -> yes, 2 -> no) (currently it is set to '%s') ", currMode);

            int change;
            const int res = readInt(prompt, &change);
            if (res == -1) {
                return clientExit(cli);
            }

            if (!res || (change != 1 && change != 2)) {
                printf("\033[31mInvalid option selected.\033[0m\n");
                continue;
            }
            if (change == 1) {
                if (readModeFromUser(cli, &mode) != 0) {
                    return clientExit(cli);
                }
            }
            break;
        }
    } else if (programMode == 4) {
        printf("Ending program.\n");
        return 0;
    }

    if (mode == 1) {
        while (1) {
            const int res = readInt("Choose printing mode (1 -> probabilities, 2 -> steps count) ", &printMode);
            if (res == -1) {
                return clientExit(cli);
            }

            if (!res || (printMode != 1 && printMode != 2)) {
                printf("\033[31mInvalid printing mode selected.\033[0m\n");
                continue;
            }
            break;
        }
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
    } else if (mode == 2) {
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
            if (ipcClientSend(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
                printf("\033[31mSend failed (header).\033[0m\n");
                ipcClientClose(&cli);
                return 1;
            }

            StartPositionRequest startReq = {0};
            startReq.startX = x;
            startReq.startY = y;

            if (ipcClientSend(&cli, (char *) &startReq, sizeof(StartPositionRequest)) <= 0) {
                printf("\033[31mSend failed (starting position).\033[0m\n");
                ipcClientClose(&cli);
                return 1;
            }
            printf("\nClient sent starting position request.\n");

            StartPositionResult st = {0};
            const int r = ipcClientRecv(&cli, (char *) &st, sizeX * sizeY * sizeof(StartPositionResult));
            if (r <= 0) {
                printf("\033[31mReceive failed (starting position result).\033[0m\n");
                ipcClientClose(&cli);
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

    h.type = MSG_SIMULATION;
    if (ipcClientSend(&cli, (char *) &h, sizeof(MessageHeader)) <= 0) {
        printf("\033[31mSend failed (header).\033[0m\n");
        ipcClientClose(&cli);
        return 1;
    }

    if (!load) {
        SimRequest req = {0};
        req.p_up = up;
        req.p_down = down;
        req.p_left = left;
        req.p_right = right;
        req.maxSteps = K;

        if (ipcClientSend(&cli, (char *) &req, sizeof(SimRequest)) <= 0) {
            printf("\033[31mSend failed (simulation).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }
    }
    if (mode == 1) {
        ReplicationRequest req = {0};
        req.replications = replications;
        req.printMode = printMode;

        if (ipcClientSend(&cli, (char *) &req, sizeof(ReplicationRequest)) <= 0) {
            printf("\033[31mSend failed (replications).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }
    }

    printf("\nClient sent simulation request.\n");

    if (mode == 1) {
        WalkResult res[sizeX][sizeY];
        memset(res, 0, sizeof res);

        const int r = ipcClientRecv(&cli, (char *) &res, sizeX * sizeY * sizeof(WalkResult));
        if (r <= 0) {
            printf("\033[31mReceive failed (simulation result).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }

        printf("\nClient received simulation result.\n");

        drawResultMap(stdout, sizeX, sizeY, res, printMode);
    } else if (mode == 2) {
        WalkPathResult res = {0};

        const int r = ipcClientRecv(&cli, (char *) &res, sizeof(WalkPathResult));
        if (r <= 0) {
            printf("\033[31mReceive failed (simulation result).\033[0m\n");
            ipcClientClose(&cli);
            return 1;
        }

        printf("\nClient received simulation result.\n");

        drawPath(stdout, res);
    }

    clientExit(cli);
}
