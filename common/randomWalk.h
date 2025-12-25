#ifndef PROJECTS_RANDOM_WALK_H
#define PROJECTS_RANDOM_WALK_H
#include "world.h"

#define MAX_PATH 2048
#define MAX_WORLD 4096

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
    Position path[MAX_PATH];
    char world[MAX_WORLD];
} WalkPathResult;

WalkResults randomWalkReplications(Position start, Probabilities pr, int K, int count, World world);
WalkPathResult randomWalkWithPath(Position start, Probabilities pr, int K, World world);

#endif