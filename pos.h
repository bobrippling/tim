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

/* swap points to order y */
void point_sort_y(point_t *a, point_t *b);

/* order y and x, regardless of a/b */
void point_sort_full(point_t *a, point_t *b);

/* order points and x, if a.y==b.y */
void point_sort_yx(point_t *a, point_t *b);

#define point_eq(a, b) !memcmp((a), (b), sizeof(point_t))

#endif
