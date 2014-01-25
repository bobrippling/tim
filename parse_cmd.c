#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <wordexp.h>

#include "mem.h"

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"
#include "pos.h"
#include "region.h"
#include "range.h"

#include "parse_cmd.h"

static
char *parse_arg(const char *arg)
{
	/* TODO: ~ substitution */
	wordexp_t wexp;
	memset(&wexp, 0, sizeof wexp);

	int r = wordexp(arg, &wexp, WRDE_NOCMD);

	char *ret = r
		? ustrdup(arg)
		: join(" ", (const char **)wexp.we_wordv, wexp.we_wordc);

	wordfree(&wexp);

	return ret;
}

bool parse_cmd(
		char *cmd,
		int *pargc, char ***pargv,
		bool *force,
		struct range **range)
{
	switch(parse_range(cmd, &cmd, *range)){
		case RANGE_PARSE_FAIL:
			return false;
		case RANGE_PARSE_NONE:
			*range = NULL;
		case RANGE_PARSE_PASS:
			break;
	}

	int argc = *pargc;
	char **argv = *pargv;
	char *p;
	bool had_punct;

	/* special case */
	if((had_punct = ispunct(cmd[0]))){
		argv = urealloc(argv, (argc + 2) * sizeof *argv);

		snprintf(
				argv[argc++] = umalloc(2),
				2, "%s", cmd);

		cmd++;
	}

	for(p = strtok(cmd, " "); p; p = strtok(NULL, " ")){
		argv = urealloc(argv, (argc + 2) * sizeof *argv);
		argv[argc++] = parse_arg(p);
	}
	if(argv)
		argv[argc] = NULL;

	/* special case: check for '!' in the first cmd */
	if(!had_punct && argc >= 1 && (p = strchr(argv[0], '!'))){
		*force = true;
		*p = '\0';
		if(p[1]){
			/* split, e.g. "w!hello" -> "w", "hello" */
			argv = urealloc(argv, (++argc + 1) * sizeof *argv);
			for(int i = argc - 1; i > 1; i--)
				argv[i] = argv[i - 1];

			argv[1] = ustrdup(p + 1);
		}
	}

	*pargv = argv;
	*pargc = argc;

	return true;
}

void filter_cmd(int *pargc, char ***pargv)
{
	/* check for '%' */
	int argc = *pargc;
	char **argv = *pargv;
	int i;
	const char *const fnam = buffer_fname(buffers_cur());


	for(i = 0; i < argc; i++){
		char *p;

		for(p = argv[i]; *p; p++){
			if(*p == '\\')
				continue;

			switch(*p){
				/* TODO: '#' */
				case '%':
					if(fnam){
						const int di = p - argv[i];
						char *new;

						*p = '\0';

						new = join("", (const char *[]){
								argv[i],
								fnam,
								p + 1 }, 3);

						free(argv[i]);
						argv[i] = new;
						p = argv[i] + di;
					}
					break;
			}
		}
	}
}

