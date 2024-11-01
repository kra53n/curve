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

int ftoa(float f, char *buf)
{
	char *ptr = buf;
	char c;
    int precision = 8;

	if (f < 0)
	{
		f = -f;
		*ptr++ = '-';
	}

    *ptr++ = (int)f + '0';
    f -= (int)f;

    *ptr++ = '.';

    while (precision--)
    {
        f *= 10.0;
        c = f;
        *ptr++ = c + '0';
        f -= c;
    }
	*ptr = 0;
	return ptr - buf;
}

#endif // UTILS_
