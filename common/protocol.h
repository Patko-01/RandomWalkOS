#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct {
    double p_up;
    double p_down;
    double p_left;
    double p_right;

    int maxSteps;     // K
    int replications;
} SimRequest;

typedef struct {
    int mode; // 1 = summary, 2 = interactive
} ModeRequest;

typedef struct {
    int startX;
    int startY;
} StartPositionRequest;

typedef struct {
    int notOk;
} StartPositionResult;

typedef struct {
    int obstaclesMode;
    int sizeX;
    int sizeY;
} MapRequest;

typedef struct {
    int end;
} EndRequest;

#endif
