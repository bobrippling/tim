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
		char *str;
		struct ent *next;
	} ents[HASH_CNT];
};

static unsigned hash_str(const char *s)
{
	unsigned hash = 0;

	for(; *s; s++)
		hash = *s + (hash << 6) + (hash << 16) - hash;

	return hash;
}

static struct ent *hash_get(struct hash *h, char *s, bool *exists)
{
	struct ent *e = &h->ents[hash_str(s) % HASH_CNT];

	for(;;){
		if(e->str && !strcmp(e->str, s)){
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

struct hash *hash_new()
{
	return umalloc(sizeof *hash_new());
}

bool hash_add(struct hash *h, char *s)
{
	bool exists;
	struct ent *e = hash_get(h, s, &exists);

	if(exists)
		return false;

	if(e->str){
		/* different string, next entry */
		assert(!e->next);
		e->next = umalloc(sizeof *e->next);
		e = e->next;
	}

	e->str = s;

	return true;
}

bool hash_exists(struct hash *h, char *s)
{
	bool exists;
	hash_get(h, s, &exists);
	return exists;
}

static void ent_free(struct ent *e)
{
	if(e){
		struct ent *n = e->next;
		free(e->str);
		free(e);

		ent_free(n);
	}
}

void hash_free(struct hash *h)
{
	if(!h)
		return;

	for(int i = 0; i < HASH_CNT; i++){
		struct ent *e = &h->ents[i];
		free(e->str);
		ent_free(e->next);
	}

	free(h);
}

char *hash_ent(struct hash *h, unsigned i)
{
	unsigned hidx = 0;
	struct ent *e = &h->ents[hidx];

	while(1){
		if(e->str && i-- == 0)
			return e->str;

		if(!(e = e->next)){
			if(++hidx == HASH_CNT)
				return NULL;

			e = &h->ents[hidx];
		}
	}
}
