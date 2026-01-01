#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "randomWalk.h" // kvoli makram

typedef enum {
    MSG_EXIT,
    MSG_MODE,
    MSG_MAP,
    MSG_START_POS,
    MSG_SIMULATION
} MessageType;

typedef struct {
    MessageType type;
} MessageHeader;

typedef struct {
    int mode; // 1 = summary, 2 = interactive
} ModeRequest;

typedef struct {
    int obstaclesMode;
    int sizeX;
    int sizeY;
} MapRequest;

typedef struct {
    int startX;
    int startY;
} StartPositionRequest;

typedef struct {
    int notOk;
} StartPositionResult;

typedef struct {
    char world[MAX_WORLD_X][MAX_WORLD_Y];
} WorldRequest;

typedef struct {
    double p_up;
    double p_down;
    double p_left;
    double p_right;

    int maxSteps;     // K
    int replications;
} SimRequest;

#endif
