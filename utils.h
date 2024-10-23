#ifndef UTILS_
#define UTILS_

#include "raylib.h"

void drawr(Rectangle r, Color c) {
    if (c.a >= 200)
        c.a = 255 / 5;
    DrawRectangleRec(r, c);
}

void drawc(int x, int y, float r, Color c) {
    if (c.a >= 200)
        c.a = 255 / 5;
    DrawCircle(x, y, r, c);
}

#endif // UTILS_
