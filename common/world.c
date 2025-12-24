#include "world.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//TODO: dorob prekazky

World createWorld(const int sizeX, const int sizeY, const int startX, const int startY) {
    World w;

    w.sizeX = sizeX;
    w.sizeY = sizeY;
    w.startX = startX;
    w.startY = startY;

    w.worldBuffer = malloc(sizeX * sizeY * sizeof(char));
    memset(w.worldBuffer, '.', sizeX * sizeY);

    WORLD_AT(&w, w.startX, w.startY) = 'O';

    if (w.worldBuffer == NULL) {
        printf("Failed to allocate world.\n");
        w.sizeX = 0;
        w.sizeY = 0;
    }

    return w;
}

void placeObstacles(const World world) {
    int randX = (rand() % (world.sizeX - 1)) + 1;
    int randY = (rand() % (world.sizeY - 1)) + 1;

    while (randX == world.startX && randY == world.startY) {
        randX = (rand() % (world.sizeX - 1)) + 1;
        randY = (rand() % (world.sizeY - 1)) + 1;
    }

    WORLD_AT(&world, randX, randY) = '#';
}

void destroyWorld(World *w) {
    free(w->worldBuffer);
    w->worldBuffer = NULL;
    w->sizeX = 0;
    w->sizeY = 0;
}

