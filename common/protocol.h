#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_FILE_NAME 1024
#define MAX_PATH 1024
#define MAX_WORLD_X 50
#define MAX_WORLD_Y 50

typedef enum {
    MSG_EXIT,
    MSG_MODE,
    MSG_MAP,
    MSG_START_POS,
    MSG_SIMULATION,
    MSG_LOAD,
    MSG_FILE
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
    char filename[MAX_FILE_NAME];
} FileRequest;

typedef struct {
    double p_up;
    double p_down;
    double p_left;
    double p_right;

    int maxSteps;     // K
} SimRequest;

typedef struct {
    int replications;
    int printMode;
} ReplicationRequest;

typedef struct {
    MapRequest mapReq;
    WorldRequest wReq;
    SimRequest sReq;
} LoadedResponse;

typedef struct {
    double probSuccess;
    double avgStepCount;
} WalkResult;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    int pathLen;
    int worldX;
    int worldY;

    int success;
    int stuck;

    Position path[MAX_PATH];
    char world[MAX_WORLD_X][MAX_WORLD_Y];
} WalkPathResult;

#endif
