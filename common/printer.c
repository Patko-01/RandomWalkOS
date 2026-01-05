#include "printer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void drawWorldWithWalker(FILE *out, const WalkPathResult result, const int walkerX, const int walkerY) {
    for (int y = 0; y < result.worldY; ++y) {
        for (int x = 0; x < result.worldX; ++x) {
            if (x == walkerX && y == walkerY) {
                fputc('@', out); // walker
            } else {
                fputc(result.world[x][y], out);
            }
        }
        fputc('\n', out);
    }
}

void drawPath(FILE *out, const WalkPathResult result) {
    for (int i = 0; i < result.pathLen; ++i) {
        fprintf(out,"Step %d / %d : (%d, %d)\n", i, result.pathLen - 1, result.path[i].x, result.path[i].y);

        if (i > 0) {
            char where[28];

            if (result.path[i - 1].x < result.path[i].x) {
                if (result.path[i].x - result.path[i - 1].x > 1) {
                    // presiel za okraj
                    strcpy(where, "'left' (crossed the world)");
                } else {
                    strcpy(where, "'right'");
                }
            } else if (result.path[i - 1].x > result.path[i].x) {
                if (result.path[i - 1].x - result.path[i].x > 1) {
                    // presiel za okraj
                    strcpy(where, "'right' (crossed the world)");
                } else {
                    strcpy(where, "'left'");
                }
            } else if (result.path[i - 1].y < result.path[i].y) {
                if (result.path[i].y - result.path[i - 1].y > 1) {
                    // presiel za okraj
                    strcpy(where, "'up' (crossed the world)");
                } else {
                    strcpy(where, "'down'");
                }
            } else if (result.path[i - 1].y > result.path[i].y) {
                if (result.path[i - 1].y - result.path[i].y > 1) {
                    // presiel za okraj
                    strcpy(where, "'down' (crossed the world)");
                } else {
                    strcpy(where, "'up'");
                }
            }

            fprintf(out, "Walker moved %s.\n", where);
        }

        drawWorldWithWalker(out, result, result.path[i].x, result.path[i].y);
        fputc('\n', out);

        usleep(200000); // 200 ms
    }

    if (result.success) {
        fprintf(out, "Walker successfully walked to (0, 0).\n\n");
    } else if (result.stuck) {
        fprintf(out, "Walker got stuck.\n\n");
    }
}

// VLA
void drawResultMap(FILE *out, const int sizeX, const int sizeY, const WalkResult results[sizeX][sizeY], const int printMode) {
    double highestAvgStepCount = 0;

    for (int y = 0; y < sizeY; ++y) {
        for (int x = 0; x < sizeX; ++x) {
            if (results[x][y].avgStepCount > highestAvgStepCount) {
                highestAvgStepCount = results[x][y].avgStepCount;
            }
        }
    }

    int mostDigits = 1;
    if (printMode == 2) {
        if ((int) highestAvgStepCount > 0) {
            mostDigits = (int) log10(abs((int) highestAvgStepCount)) + 1;
        }

        if (mostDigits < 2) {
            mostDigits = 2;
        }

        mostDigits += 3; // lebo 2 desatinne miesta + bodka
    }

    for (int y = 0; y < sizeY; ++y) {
        for (int x = 0; x < sizeX; ++x) {
            int currCellWidth = 5; // default v pripade W, # alebo -1.00

            if (x == 0 && y == 0) {
                fprintf(out, "  W  ");
            } else if (results[x][y].avgStepCount == 0) {
                fprintf(out, "  #  ");
            } else {
                if (printMode == 1) {
                    fprintf(out, "%.2f ", results[x][y].probSuccess);
                } else if (printMode == 2) {
                    fprintf(out, "%.2f", results[x][y].avgStepCount);

                    if (results[x][y].avgStepCount > 0) {
                        currCellWidth = (int) log10(abs((int) results[x][y].avgStepCount)) + 1;
                        currCellWidth += 3;
                    }
                }
            }

            if (printMode == 2) {
                fprintf(out, "%*s", (mostDigits - currCellWidth) + 1, ""); // +1 pre medzeru medzi vypismi
            }
        }
        fputc('\n', out);
    }

    fprintf(out, "Highest average step count: %.4f\n\n", highestAvgStepCount);
}