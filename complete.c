#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stdio.h>

#include "complete.h"
#include "hash.h"

#include "str.h"
#include "mem.h"

struct hash_str_ent
{
	char *str; /* must be first - hash expects strings */
	bool hidden;
};

bool complete_1_ishidden(void *p)
{
	struct hash_str_ent *e = p;
	return e->hidden;
}

char *complete_1_getstr(void *p)
{
	struct hash_str_ent *e = p;
	return e->str;
}

static unsigned hash_str_ent(const struct hash_str_ent *e)
{
	unsigned hash = 0;
	char *s = e->str;

	for(; *s; s++)
		hash = *s + (hash << 6) + (hash << 16) - hash;

	return hash;
}

static bool cmp_str_ent(
		const struct hash_str_ent *a,
		const struct hash_str_ent *b)
{
	return !strcmp(a->str, b->str);
}

bool complete_init(struct complete_ctx *ctx, char *line, unsigned len, int x)
{
	memset(ctx, 0, sizeof *ctx);

	if((unsigned)x > len)
		return false;

	ctx->current = word_before(line, x);
	ctx->current_len = strlen(ctx->current);

	ctx->ents = hash_new(
			(hash_fn *)&hash_str_ent,
			(hash_eq_fn *)&cmp_str_ent);

	return true;
}

static void hash_ent_free(struct hash_str_ent *ent)
{
	free(ent->str);
	free(ent);
}

void complete_teardown(struct complete_ctx *c)
{
	hash_free(c->ents, (void (*)(void *))&hash_ent_free);
	free(c->current);
}

void complete_gather(char *const line, size_t const line_len, void *c)
{
	/* XXX: lines with nuls in are ignored after the nul */
	struct complete_ctx *ctx = c;

	char *found = tim_strstr(line, line_len, ctx->current);
	while(found){
		/* make sure it's a word start */
		if(found > line && isalnum(found[-1])){
			/* substring of another word, ignore */
			goto next;
		}

		char *end;
		for(end = found + ctx->current_len; isalnum(*end); end++);

		struct hash_str_ent *new = umalloc(sizeof *new);
		new->str = ustrdup_len(found, end - found);

		if(!hash_add(ctx->ents, new)){
			free(new->str);
			free(new);
		}

next:
		found = tim_strstr(end, line_len - (end - line), ctx->current);
	}
}

void complete_filter(struct complete_ctx *c, int newch)
{
	// TODO: handle backspace
	c->current = urealloc(c->current, ++c->current_len);
	c->current[c->current_len - 1] = newch;

	struct hash_str_ent *ent;
	size_t i = 0;
	for(; (ent = hash_ent(c->ents, i)); i++){
		if(!ent->hidden
		&& strncmp(ent->str, c->current, c->current_len))
		{
			ent->hidden = true;
		}
	}
}
