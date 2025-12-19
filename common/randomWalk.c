#include "randomWalk.h"
#include <stdio.h>
#include <stdlib.h>

int nextStep(Probabilities pr) {
    double r = (double)rand()/RAND_MAX;

    if (r < pr.p_up) {
        return 0; //up
    } else if (r < pr.p_up + pr.p_down) {
        return 1; //down
    } else if (r < pr.p_up + pr.p_down + pr.p_left) {
        return 2; //left
    } else {
        return 3; //right
    }
}

int randomWalk(Position start, Probabilities pr, int K) {
    Position pos = start; //copy
    int steps = 0;

    for (int i = 0; i < K; ++i) {
        if (pos.x == 0 && pos.y == 0) {
            return steps;
        }

        const int direction = nextStep(pr);

        switch (direction) {
            case 0: ++pos.y; break;
            case 1: --pos.y; break;
            case 2: --pos.x; break;
            case 3: ++pos.x; break;
            default: ;
        }

        ++steps;
    }
    return -1;
}

WalkResult randomWalkReplications(Position start, Probabilities pr, int K, int count) {
    WalkResult result = {-1, -1};
    int sumSteps = 0;
    int successCount = 0;

    for (int i = 0; i < count; ++i) {
        const int steps = randomWalk(start, pr, K);

        if (steps != -1) {
            sumSteps += steps;
            ++successCount;
        }
    }

    if (successCount != 0) {
        result.avgStepCount = (double) sumSteps / successCount;
        result.probSuccess = (double) successCount / count;
    }

    return result;
}