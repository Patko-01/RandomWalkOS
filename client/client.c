#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "../common/ipc.h"
#include "../common/protocol.h"
#include "../common/randomWalk.h"

// tento kód mám od AI
void clear_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int read_int(const char* prompt, int* out) {
    printf("%s", prompt);
    if (scanf("%d", out) != 1 || *out <= 0) {
        clear_input();
        return 0;
    }
    return 1;
}

int read_double(const char* prompt, double* out) {
    printf("%s", prompt);
    if (scanf("%lf", out) != 1 || *out <= 0) {
        clear_input();
        return 0;
    }
    return 1;
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
        int x, y, K, replications, sizeX, sizeY, wantPath;
        double up, down, left, right;

        while (!successful) {
            printf("\nPress Y to continue or N to quit. ");

            char ch;
            if (scanf(" %c", &ch) != 1) {
                printf("Error while reading user input.\n");
                clear_input();
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
                clear_input();
                successful = 0;
                continue;
            }

            if (!read_int("Choose simulation mode (1 -> summary, 2 -> interactive) ", &wantPath) ||
                (wantPath != 1 && wantPath != 2)) {
                printf("Invalid mode selected.\n");
                successful = 0;
                continue;
            }

            printf("Enter world size...\n");
            if ((!read_int("Enter X length: ", &sizeX) ||
                !read_int("Enter Y length: ", &sizeY)) ||
                (sizeX < 2 || sizeY < 2)) {
                printf("Invalid size input, world must be at least 2x2.\n");
                successful = 0;
                continue;
            }

            printf("\nWorld size set to %dx%d.\n", sizeX, sizeY);

            printf("Enter starting position...\n");
            if (!read_int("Enter starting X: ", &x) ||
                !read_int("Enter starting Y: ", &y)) {
                printf("Invalid position input.\n");
                successful = 0;
                continue;
            } else if (x >= sizeX || y >= sizeY) {
                printf("Starting position can't be outside the world.\n");
                successful = 0;
                continue;
            }

            printf("\nStarting position set to [%d, %d].\n", x, y);

            printf("\nEnter direction probabilities...\n");
            if (!read_double("Enter prob up: ", &up) ||
                !read_double("Enter prob down: ", &down) ||
                !read_double("Enter prob left: ", &left) ||
                !read_double("Enter prob right: ", &right)) {
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

            if (!read_int("Enter max steps K: ", &K) || K <= 0) {
                printf("K must be > 0.\n");
                successful = 0;
                continue;
            }
            if (!read_int("Enter replications count: ", &replications) || replications <= 0) {
                printf("Replications must be > 0.\n");
                successful = 0;
                continue;
            }

            successful = 1;
        }

        SimRequest req;
        req.wantPath = wantPath;
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

        if (wantPath == 1) {
            WalkResults res;

            const int r = ipc_client_recv(&cli, (char*)&res, sizeof(WalkResults));
            if (r <= 0) {
                printf("Receive failed.\n");
                ipc_client_close(&cli);
                return 1;
            }

            printf("\nClient received simulation result.\n");
            printf("Average steps: %.2f\n", res.avgStepCount);
            printf("Success probability: %.2f\n", res.probSuccess);

            if (res.probSuccess == -1) {
                printf("Person in the simulation never reached [0, 0].\n");
            }
        } else {
            WalkPathResult res;

            const int r = ipc_client_recv(&cli, (char*)&res, sizeof(WalkPathResult));
            if (r <= 0) {
                printf("Receive failed.\n");
                ipc_client_close(&cli);
                return 1;
            }

            // TODO: vykreslenie
            for (int i = 0; i < res.pathLen; i++) {
                printf("Step %d: (%d, %d)\n",
                       i,
                       res.path[i].x,
                       res.path[i].y);
                usleep(200000); // 200 ms
            }
        }
    }
}