#ifndef PROJECTS_WORLD_H
#define PROJECTS_WORLD_H

#define WORLD_AT(w, x, y) ((w)->worldBuffer[(y) * (w)->sizeX + (x)]) //tento kod je od AI

typedef struct {
    int sizeX;
    int sizeY;
    int startX;
    int startY;

    char* worldBuffer;
} World;

World createWorld(int sizeX, int sizeY, int startX, int startY);
int isSafeToStart(World world, int x, int y);
void placeObstacles(const World *world);
void destroyWorld(World *w);

#endif