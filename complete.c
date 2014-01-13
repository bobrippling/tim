#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "complete.h"
#include "hash.h"

#include "str.h"
#include "mem.h"

bool complete_init(struct complete_ctx *ctx, char *line, unsigned len, int x)
{
	memset(ctx, 0, sizeof *ctx);

	if((unsigned)x > len)
		return false;

	ctx->current = word_before(line, x);

	ctx->ents = hash_new();

	return true;
}

void complete_teardown(struct complete_ctx *c)
{
	hash_free(c->ents);
	free(c->current);
}

void complete_gather(char *line, void *c)
{
	/* XXX: lines with nuls in are ignored after the nul */
	struct complete_ctx *ctx = c;

	char *found = strstr(line, ctx->current);
	while(found){
		char *end;
		for(end = found + ctx->current_len; isalnum(*end); end++);

		/* make sure it's a word start */
		if(found > line && isalnum(found[-1])){
			/* substring of another word, ignore */
			goto next;
		}

		char *new = ustrdup_len(found, end - found);

		if(!hash_add(ctx->ents, new))
			free(new);

next:
		found = strstr(end, ctx->current);
	}
}
