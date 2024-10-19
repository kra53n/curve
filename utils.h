#ifndef UTILS_
#define UTILS_

#include "raylib.h"

void drawr(Rectangle r, Color c) {
    c.a >>= 4;
    DrawRectangleRec(r, c);
}

#endif // UTILS_
