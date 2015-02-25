#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stdio.h>

#include "complete.h"
#include "hash.h"
#include "keymacros.h" /* case_BACKSPACE */

#include "str.h"
#include "mem.h"

struct hash_str_ent
{
	char *str; /* must be first - hash expects strings */
	bool hidden;
};

bool complete_1_ishidden(const void *p)
{
	const struct hash_str_ent *e = p;
	return e->hidden;
}

bool complete_1_isvisible(const void *p)
{
	return !complete_1_ishidden(p);
}

char *complete_1_getstr(const void *p)
{
	const struct hash_str_ent *e = p;
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

	ctx->current_word = word_before(line, x);
	ctx->current_word_len = strlen(ctx->current_word);

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
}

void complete_gather(char *const line, size_t const line_len, void *c)
{
	/* XXX: lines with nuls in are ignored after the nul */
	struct complete_ctx *ctx = c;

	char *const line_end = line + line_len;

	char *found = tim_strstr(line, line_len, ctx->current_word);
	while(found){

		char *end;
		for(end = found + ctx->current_word_len;
				end < line_end && isalnum(*end);
				end++);

		/* make sure it's a word start */
		if(found > line && isalnum(found[-1])){
			/* substring of another word, ignore */
		}else if((size_t)(end - found) <= ctx->current_word_len){
			/* too short */
		}else{
			struct hash_str_ent *new = umalloc(sizeof *new);
			new->str = ustrdup_len(found, end - found);

			if(!hash_add(ctx->ents, new)){
				free(new->str);
				free(new);
			}
		}

		found = tim_strstr(end, line_len - (end - line), ctx->current_word);
	}
}

void complete_filter(struct complete_ctx *c, int newch)
{
	switch(newch){
		case_BACKSPACE:
			if(c->current_word_len){
				c->current_word_len--;
				c->current_word[c->current_word_len] = '\0';

				/* TODO: if c->current_word_len <= c->recalc_len,
				 * then recalculate words for the new word */

				struct hash_str_ent *ent;
				size_t i = 0;
				for(; (ent = hash_ent(c->ents, i)); i++)
					ent->hidden = false;
			}else{
				/* TODO - cancel */
			}
			break;

		default:
			c->current_word_len++;
			c->current_word = urealloc(c->current_word, c->current_word_len + 1);
			c->current_word[c->current_word_len - 1] = newch;
			c->current_word[c->current_word_len] = '\0';
	}

	struct hash_str_ent *ent;
	size_t i = 0;

	for(; (ent = hash_ent(c->ents, i)); i++){
		if(!ent->hidden
		&& strncmp(ent->str, c->current_word, c->current_word_len))
		{
			ent->hidden = true;
		}
	}
}

void *complete_hash_ent(struct hash *h, size_t i)
{
	for(;;){
		void *ent = hash_ent(h, i);

		if(!ent)
			return NULL;

		if(complete_1_ishidden(ent)){
			i++;
			continue;
		}

		return ent;
	}
}
