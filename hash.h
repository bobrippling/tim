#ifndef HASH_H
#define HASH_H

#include <stdbool.h>

struct hash;
typedef unsigned hash_fn(const void *);
typedef bool hash_eq_fn(const void *, const void *);

struct hash *hash_new(hash_fn, hash_eq_fn);

bool hash_add(struct hash *, void *);
bool hash_exists(struct hash *, void *);

void hash_free(struct hash *, void fn(void *));

void *hash_ent(struct hash *, unsigned);

size_t hash_cnt(struct hash *);

#endif
