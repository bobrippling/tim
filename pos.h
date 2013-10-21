#ifndef RECT_H
#define RECT_H

typedef struct rect  rect_t;
typedef struct point point_t;

struct rect
{
	int x, y, w, h;
};

struct point
{
	int x, y;
};

void point_minmax(point_t *, point_t *);

#endif
