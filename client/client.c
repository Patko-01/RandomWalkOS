#include <stdio.h>
#include <math.h>

#include "../common/ipc.h"
#include "../common/protocol.h"
#include "../common/randomWalk.h"

int read_int(const char* prompt, int* out) {
    printf("%s", prompt);
    if (scanf("%d", out) != 1) {
        return 0;
    }
    return 1;
}

int read_double(const char* prompt, double* out) {
    printf("%s", prompt);
    if (scanf("%lf", out) != 1) {
        return 0;
    }
    return 1;
}

int main(void) {
    ipc_client cli;
    if (ipc_client_connect(&cli, IPC_PORT) != 0) {
        fprintf(stderr, "Client connect failed\n");
        return 1;
    }
    printf("Client connected to port %s.\n", IPC_PORT);

    printf("Enter starting position...\n");
    int x, y;
    if (!read_int("Enter starting X: ", &x) ||
        !read_int("Enter starting Y: ", &y)) {
        fprintf(stderr, "Invalid position input\n");
        return 1;
    }
    printf("\nStarting position set to [%d, %d].\n", x, y);

    printf("\nEnter direction probabilities...\n");
    double up, down, left, right;
    if (!read_double("Enter prob up: ", &up) ||
        !read_double("Enter prob down: ", &down) ||
        !read_double("Enter prob left: ", &left) ||
        !read_double("Enter prob right: ", &right)) {
        fprintf(stderr, "Invalid probability input\n");
        return 1;
    }
    double sum = fabs(up + down + left + right);
    if (sum < 0.999 || sum > 1.001) { // lebo sucet double nikdy nie je presne 1
        fprintf(stderr, "Probabilities must be >= 0 and sum to 1\n");
        return 1;
    }
    printf("\nDirection probabilities set to {up: %lf, down: %lf, left: %lf, right: %lf}.\n\n", up, down, left, right);

    int K, replications;
    if (!read_int("Enter max steps K: ", &K) || K <= 0) {
        fprintf(stderr, "K must be > 0\n");
        return 1;
    }
    if (!read_int("Enter replications: ", &replications) || replications <= 0) {
        fprintf(stderr, "Replications must be > 0\n");
        return 1;
    }

    SimRequest req;
    req.startX = x;
    req.startY = y;
    req.p_up = up;
    req.p_down = down;
    req.p_left = left;
    req.p_right = right;
    req.maxSteps = K;
    req.replications = replications;

    if (ipc_client_send(&cli, (char*)&req, sizeof(SimRequest)) <= 0) {
        fprintf(stderr, "Send failed\n");
        ipc_client_close(&cli);
        return 1;
    }
    printf("\nClient sent simulation request.\n");

    WalkResult res;

    int r = ipc_client_recv(&cli, (char*)&res, sizeof(res));
    if (r <= 0) {
        fprintf(stderr, "Recv failed\n");
        ipc_client_close(&cli);
        return 1;
    } else if (r != sizeof(WalkResult)) {
        fprintf(stderr, "Received wrong response from client.\n");
        ipc_client_close(&cli);
        return 1;
    }

    printf("\nClient received simulation result.\n");
    printf("Average steps: %.2f\n", res.avgStepCount);
    printf("Success probability: %.2f\n", res.probSuccess);

    if (res.probSuccess == -1) {
        printf("Person in the simulation never reached [0, 0].\n");
    }

    ipc_client_close(&cli);
    return 0;
}
