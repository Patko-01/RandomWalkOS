#ifndef PROJECTS_RANDOM_WALK_H
#define PROJECTS_RANDOM_WALK_H

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
} WalkResult;

int randomWalk(Position start, Probabilities pr, int K);
WalkResult randomWalkReplications(Position start, Probabilities pr, int K, int count);

#endif