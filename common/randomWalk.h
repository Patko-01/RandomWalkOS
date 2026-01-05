#ifndef PROJECTS_RANDOM_WALK_H
#define PROJECTS_RANDOM_WALK_H
#include "world.h"
#include "protocol.h"

typedef struct {
    double p_up;
    double p_down;
    double p_left;
    double p_right;
} Probabilities;

WalkResult randomWalkReplications(Position start, Probabilities pr, int K, int count, World *world);
WalkPathResult randomWalkWithPath(Position start, Probabilities pr, int K, const World *world);

#endif