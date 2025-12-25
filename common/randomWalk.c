#include "randomWalk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    int success;
    int stepCount;
    int K;

    Position start;
    Probabilities pr;
    World world;

    pthread_mutex_t mutex;
} Shared;

int nextStep(const Probabilities pr, unsigned int *seed) {
    const double r = (double)rand_r(seed) / RAND_MAX;

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

int randomWalk(const Position start, const Probabilities pr, const int K, const World world) {
    unsigned int seed = (unsigned int)pthread_self();
    int currX = start.x;
    int currY = start.y;
    int steps = 0;

    for (int i = 0; i < K; ++i) {
        if (currX == 0 && currY == 0) {
            return steps;
        }

        do {
            const int direction = nextStep(pr, &seed);

            switch (direction) {
                case 0: ++currY; break;
                case 1: --currY; break;
                case 2: --currX; break;
                case 3: ++currX; break;
                default: ;
            }
        } while (WORLD_AT(&world, currX, currY) == '#');

        if (currX < 0) {
            currX = world.sizeX - 1;
        } else if (currX >= world.sizeX) {
            currX = 0;
        }

        if (currY < 0) {
            currY = world.sizeY - 1;
        } else if (currY >= world.sizeY) {
            currY = 0;
        }

        ++steps;
    }

    return -1;
}

void *randomWalkRoutine(void* arg) {
    Shared *sh = (Shared*) arg;

    const int steps = randomWalk(sh->start, sh->pr, sh->K, sh->world);

    if (steps != -1) {
        pthread_mutex_lock(&sh->mutex);
        sh->stepCount += steps;
        sh->success++;
        pthread_mutex_unlock(&sh->mutex);
    }

    return NULL;
}

WalkResults randomWalkReplications(const Position start, const Probabilities pr, const int K, const int count, const World world) {
    pthread_t th[count];

    Shared sh;
    sh.start = start;
    sh.pr = pr;
    sh.K = K;
    sh.stepCount = 0;
    sh.success = 0;
    sh.world = world;

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

WalkPathResult randomWalkWithPath(const Position start, const Probabilities pr, const int K, const World world) {
    unsigned int seed = (unsigned int)pthread_self();

    WalkPathResult result;
    int currX = start.x;
    int currY = start.y;

    result.pathLen = 0;

    memset(result.path, 0, sizeof(char));
    memcpy(result.world, world.worldBuffer, world.sizeX * world.sizeY);

    for (int i = 0; i < K; ++i) {
        if (currX == 0 && currY == 0) {
            return result;
        }

        do {
            const int direction = nextStep(pr, &seed);

            switch (direction) {
                case 0: ++currY; break;
                case 1: --currY; break;
                case 2: --currX; break;
                case 3: ++currX; break;
                default: ;
            }
        } while (WORLD_AT(&world, currX, currY) == '#');

        if (currX < 0) {
            currX = world.sizeX - 1;
        } else if (currX >= world.sizeX) {
            currX = 0;
        }

        if (currY < 0) {
            currY = world.sizeY - 1;
        } else if (currY >= world.sizeY) {
            currY = 0;
        }

        result.path[result.pathLen++] = (Position) { currX, currY };
    }

    return result;
}