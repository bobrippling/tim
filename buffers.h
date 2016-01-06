#ifndef BUFFERS_H
#define BUFFERS_H

#define buffers_cur() windows_cur()->buf
buffer_t *buffers_find(const char *);

bool buffers_modified_single(const buffer_t *);

struct tab;
struct window *buffers_modified_excluding(buffer_t *excluding, struct tab **const ptab);

#endif
