#include "randomWalk.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int success;
    int stepCount;
    int K;

    Position start;
    Probabilities pr;

    pthread_mutex_t mutex;
} Shared;

int nextStep(Probabilities pr, unsigned int *seed) {
    double r = (double)rand_r(seed) / RAND_MAX;

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
    unsigned int seed = (unsigned int)pthread_self();
    Position pos = start; //copy
    int steps = 0;

    for (int i = 0; i < K; ++i) {
        if (pos.x == 0 && pos.y == 0) {
            return steps;
        }

        const int direction = nextStep(pr, &seed);

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

void *randomWalkRoutine(void* arg) {
    Shared *sh = (Shared*) arg;

    const int steps = randomWalk(sh->start, sh->pr, sh->K);

    if (steps != -1) {
        pthread_mutex_lock(&sh->mutex);
        sh->stepCount += steps;
        sh->success++;
        pthread_mutex_unlock(&sh->mutex);
    }

    return NULL;
}

WalkResults randomWalkReplications(const Position start, const Probabilities pr, const int K, int count) {
    pthread_t th[count];

    Shared sh;
    sh.start = start;
    sh.pr = pr;
    sh.K = K;
    sh.stepCount = 0;
    sh.success = 0;

    pthread_mutex_init(&sh.mutex, NULL);

    WalkResults result = {-1, -1};

    for (int i = 0; i < count; ++i) {
        if (pthread_create(&th[i], NULL, randomWalkRoutine, (void*) &sh) != 0) {
            fprintf(stderr, "Thread create failed.\n");
            return result;
        }
    }

    for (int i = 0; i < count; ++i) {
        if (pthread_join(th[i], NULL) != 0) {
            fprintf(stderr, "Thread join failed.\n");
            return result;
        }
    }

    if (sh.success != 0) {
        result.avgStepCount = (double) sh.stepCount / sh.success;
        result.probSuccess = (double) sh.success / count;
    }

    pthread_mutex_destroy(&sh.mutex);
    return result;
}