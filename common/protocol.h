#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct {
    int mode; // 1 = stats, 2 = trajectory
    int end;

    int startX;
    int startY;

    int sizeX;
    int sizeY;

    double p_up;
    double p_down;
    double p_left;
    double p_right;

    int maxSteps;     //K
    int replications;
} SimRequest;

#endif
