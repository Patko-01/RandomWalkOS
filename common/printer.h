#ifndef PROJECTS_PRINTER_H
#define PROJECTS_PRINTER_H
#include <stdio.h>
#include "protocol.h"

void drawPath(FILE *out, WalkPathResult result);
void drawResultMap(FILE *out, int sizeX, int sizeY, const WalkResult results[sizeX][sizeY], int printMode);

#endif