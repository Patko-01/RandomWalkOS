#ifndef PROJECTS_WORLD_H
#define PROJECTS_WORLD_H

#define WORLD_AT(w, x, y) ((w)->worldBuffer[(x) * (w)->sizeY + (y)]) //tento kod je od AI

typedef struct {
    int sizeX;
    int sizeY;
    int startX;
    int startY;

    char* worldBuffer;
} World;

World createWorld(int sizeX, int sizeY, int startX, int startY);
void placeObstacles(World world);
void destroyWorld(World *w);

#endif