#ifndef HASH_H
#define HASH_H

#include <stdbool.h>

struct hash;

struct hash *hash_new(void);

bool hash_add(struct hash *, char *);
bool hash_exists(struct hash *, char *);

void hash_free(struct hash *);

char *hash_ent(struct hash *, unsigned);

#endif
