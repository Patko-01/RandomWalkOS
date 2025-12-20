#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct {
    int startX;
    int startY;

    double p_up;
    double p_down;
    double p_left;
    double p_right;

    int maxSteps;     //K
    int replications;
} SimRequest;

#endif
