#include "world.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

World createWorld(const int sizeX, const int sizeY) {
    World w;

    w.sizeX = sizeX;
    w.sizeY = sizeY;

    w.worldBuffer = malloc(sizeX * sizeY * sizeof(char));
    memset(w.worldBuffer, '.', sizeX * sizeY);

    WORLD_AT(&w, 0, 0) = 'W';

    if (w.worldBuffer == NULL) {
        printf("\033[31mFailed to allocate world.\033[0m\n");
        w.sizeX = 0;
        w.sizeY = 0;
    }

    return w;
}

int isSafeToStart(const World *world, const int x, const int y) {
    if (x == 0 && y == 0) {
        return 0;
    }
    if (WORLD_AT(world, x, y) == '#') {
        return 0;
    }
    return 1;
}

void placeObstacle(const World *world, const int x, const int y) {
    const double randP = (double)rand() / (double)RAND_MAX;
    const double probObstacle = 0.65;

    if (randP < probObstacle) {
        WORLD_AT(world, x, y) = '#';
    }
}

void placeObstacles(const World *world) {
    const int maxDiagLength = world->sizeX < world->sizeY ? world->sizeX : world->sizeY;

    for (int n = 0 ; n < maxDiagLength; ++n) {
        int numOfChecked2 = 0;
        int numOfChecked3 = 0;

        for (int y = 0; y < world->sizeY; ++y) {
            for (int x = 0; x < world->sizeX; ++x) {
                if ((x == 0 && y == 0) || (x == 1 && y == 1)) { // okolie [0, 0] musi byt prejazdne
                    continue;
                }

                if (x + (2 * n) == y) {
                    if (numOfChecked2 % 2 == 0) {
                        placeObstacle(world, x, y);
                    }
                    ++numOfChecked2;
                } else if (y + (2 * n) == x) {
                    if (numOfChecked3 % 2 == 0) {
                        placeObstacle(world, x, y);
                    }
                    ++numOfChecked3;
                }
            }
        }
    }
}

void destroyWorld(World *world) {
    if (world == NULL) {
        return;
    }
    if (world->worldBuffer == NULL) {
        return;
    }

    free(world->worldBuffer);
    world->worldBuffer = NULL;
    world->sizeX = 0;
    world->sizeY = 0;
}

