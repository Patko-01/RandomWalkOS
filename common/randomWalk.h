#ifndef PROJECTS_RANDOM_WALK_H
#define PROJECTS_RANDOM_WALK_H
#include "world.h"

#define MAX_PATH 2048
#define MAX_WORLD_X 100
#define MAX_WORLD_Y 100

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    double p_up;
    double p_down;
    double p_left;
    double p_right;
} Probabilities;

typedef struct {
    double probSuccess;
    double avgStepCount;
} WalkResults;

typedef struct {
    int pathLen;
    int worldX;
    int worldY;

    Position path[MAX_PATH];
    char world[MAX_WORLD_X][MAX_WORLD_Y];
} WalkPathResult;

WalkResults randomWalkReplications(Position start, Probabilities pr, int K, int count, World world);
WalkPathResult randomWalkWithPath(Position start, Probabilities pr, int K, World world);

#endif