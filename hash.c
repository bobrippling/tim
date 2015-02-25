#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mem.h"

#include "hash.h"

#define HASH_CNT 128

struct hash
{
	struct ent
	{
		void *p;
		struct ent *next;
	} ents[HASH_CNT];

	hash_fn *hash;
	hash_eq_fn *eq;
};

static struct ent *hash_get(struct hash *h, void *p, bool *exists)
{
	struct ent *e = &h->ents[h->hash(p) % HASH_CNT];

	*exists = false;

	for(;;){
		if(e->p && h->eq(e->p, p)){
			*exists = true;
			break;
		}
		if(e->next)
			e = e->next;
		else
			break;
	}

	return e;
}

struct hash *hash_new(hash_fn hash, hash_eq_fn eq)
{
	struct hash *h = umalloc(sizeof(struct hash));
	h->hash = hash;
	h->eq = eq;
	return h;
}

bool hash_add(struct hash *h, void *p)
{
	bool exists;
	struct ent *e = hash_get(h, p, &exists);

	if(exists)
		return false;

	if(e->p){
		/* different value, next entry */
		assert(!e->next);
		e->next = umalloc(sizeof *e->next);
		e = e->next;
	}

	e->p = p;

	return true;
}

bool hash_exists(struct hash *h, void *p)
{
	bool exists;
	hash_get(h, p, &exists);
	return exists;
}

static void ent_free(struct ent *e, void fn(void *))
{
	if(e){
		struct ent *n = e->next;
		fn(e->p);
		free(e);

		ent_free(n, fn);
	}
}

void hash_free(struct hash *h, void fn(void *))
{
	if(!h)
		return;

	for(int i = 0; i < HASH_CNT; i++){
		struct ent *e = &h->ents[i];
		ent_free(e->next, fn);
	}

	free(h);
}

void *hash_ent(struct hash *h, unsigned i)
{
	unsigned hidx = 0;
	struct ent *e = &h->ents[hidx];

	while(1){
		if(e->p && i-- == 0)
			return e->p;

		if(!(e = e->next)){
			if(++hidx == HASH_CNT)
				return NULL;

			e = &h->ents[hidx];
		}
	}
}

size_t hash_cnt_filter(struct hash *h, bool include(const void *))
{
	size_t n = 0;
	size_t hidx;

	for(hidx = 0; hidx < HASH_CNT; hidx++){
		for(struct ent *e = &h->ents[hidx]; e && e->p; e = e->next){
			if(!include || include(e->p))
				n++;
		}
	}

	return n;
}

size_t hash_cnt(struct hash *h)
{
	return hash_cnt_filter(h, NULL);
}
