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

void point_sort_y(point_t *a, point_t *b);
void point_sort_all(point_t *a, point_t *b);

#endif
