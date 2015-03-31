#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <stdio.h>

#include "pos.h"
#include "complete.h"
#include "hash.h"
#include "keymacros.h" /* case_BACKSPACE */
#include "macros.h"

#include "ui.h"

#include "str.h"
#include "mem.h"

#define COMPL_MAX 10

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
		hash = tolower(*s) + (hash << 6) + (hash << 16) - hash;

	return hash;
}

static bool cmp_str_ent(
		const struct hash_str_ent *a,
		const struct hash_str_ent *b)
{
	return !strcasecmp(a->str, b->str);
}

bool complete_init(
		struct complete_ctx *ctx,
		char *line, unsigned len, int x,
		char *getter_word(char *line, int x),
		char *getter_end(char *, char *, char *, struct complete_ctx *))
{
	memset(ctx, 0, sizeof *ctx);

	if((unsigned)x > len)
		return false;

	ctx->current_word = getter_word(line, x);
	ctx->current_word_len = strlen(ctx->current_word);
	ctx->recalc_len = ctx->current_word_len;
	ctx->get_word = getter_word;
	ctx->get_end = getter_end;

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

char *complete_get_end_of_word(
		char *line, char *found, char *line_end,
		struct complete_ctx *ctx)
{
	char *end;

	for(end = found + ctx->current_word_len;
			end < line_end && iswordchar(*end);
			end++);

	return end;
}

char *complete_get_end_of_line(
		char *line, char *found, char *line_end,
		struct complete_ctx *ctx)
{
	return line_end;
}

void complete_gather(char *const line, size_t const line_len, void *c)
{
	/* XXX: lines with nuls in are ignored after the nul */
	struct complete_ctx *ctx = c;

	char *const line_end = line + line_len;

	char *found = tim_strcasestr(line, line_len, ctx->current_word);
	while(found){

		char *end = ctx->get_end(line, found, line_end, ctx);

		/* make sure it's a word start */
		if(found > line && iswordchar(found[-1])){
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

		found = tim_strcasestr(end, line_len - (end - line), ctx->current_word);
	}
}

void complete_filter(
		struct complete_ctx *c,
		int newch,
		bool *const cancel,
		bool *const recalc)
{
	*cancel = false;
	*recalc = false;

	switch(newch){
		case_BACKSPACE:
			if(c->current_word_len){
				c->current_word_len--;
				c->current_word[c->current_word_len] = '\0';

				if(c->current_word_len <= c->recalc_len){
					*recalc = true;
				}

				struct hash_str_ent *ent;
				size_t i = 0;
				for(; (ent = hash_ent(c->ents, i)); i++)
					ent->hidden = false;
			}else{
				*cancel = true;
				return;
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
		&& strncasecmp(ent->str, c->current_word, c->current_word_len))
		{
			ent->hidden = true;
		}
	}
}

void *complete_hash_ent(struct hash *h, size_t req)
{
	size_t i = 0;

	for(;;){
		void *ent = hash_ent(h, i);
		i++;

		if(!ent)
			return NULL;

		if(complete_1_ishidden(ent))
			continue; /* retry */

		/* found one */
		if(req == 0)
			return ent;

		req--;
	}
}

void complete_to_longest_common(
		struct complete_ctx *ctx,
		size_t *const n_to_insert)
{
	*n_to_insert = 0;

	void *first = complete_hash_ent(ctx->ents, 0);
	if(!first)
		return;

	char *longest = ustrdup(complete_1_getstr(first));
	size_t longest_len = strlen(longest);

	void *ent;
	for(size_t ient = 1; (ent = complete_hash_ent(ctx->ents, ient)); ient++){
		const char *const this = complete_1_getstr(ent);

		if(!*this)
			continue;

		/* first char at which they differ */
		size_t i;
		for(i = 0; i < longest_len && this[i]; i++)
			if(this[i] != longest[i])
				break;

		if(i < longest_len){
			/* they are different - truncate longest_len */
			longest_len = i;
		}
	}

	if(longest_len == 0)
		goto out;
	if(longest_len <= ctx->current_word_len)
		goto out;

	longest[longest_len] = '\0';

	*n_to_insert = longest_len - ctx->current_word_len;

	free(ctx->current_word);
	ctx->current_word = longest;
	longest = NULL;
	ctx->current_word_len = longest_len;

out:
	free(longest);
}

static void completion_calc_counts(
			struct hash *const ents,
			const int sel_or_minus1,
			int *const start, int *const num_to_show,
			bool *const show_more)
{
	const size_t total = hash_cnt_filter(ents, complete_1_isvisible);
	const bool too_many = (total > COMPL_MAX);

	if(too_many){
		*start = sel_or_minus1 - COMPL_MAX / 2;

		if(*start < 0)
			*start = 0;

		*num_to_show = COMPL_MAX;

		*show_more = ((size_t)(*start + *num_to_show) < total);

	}else{
		*start = 0;
		*num_to_show = total;
		*show_more = false;
	}
}

void complete_draw_menu(
		struct hash *ents,
		int sel_or_minus1,
		point_t const *const at)
{
	int start_i;
	int num_to_show;
	bool show_more;

	completion_calc_counts(
			ents,
			sel_or_minus1,
			&start_i, &num_to_show, &show_more);

	char **entries = umalloc(num_to_show * sizeof *entries);

	for(int i = 0; i < num_to_show; i++){
		void *ent = complete_hash_ent(ents, i + start_i);

		if(!ent){
			/* changing start has altered the count */
			num_to_show = i;
			break;
		}

		entries[i] = complete_1_getstr(ent);
	}

	ui_draw_menu(at, entries, num_to_show, sel_or_minus1 - start_i, show_more);

	free(entries);
}
