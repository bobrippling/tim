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
		: join(" ", wexp.we_wordv, wexp.we_wordc);

	wordfree(&wexp);

	return ret;
}

static void add_argv(
		char ***pargv, int *pargc,
		char **panchor, char *p)
{
#define argv (*pargv)
#define argc (*pargc)
#define anchor (*panchor)
	argv = urealloc(argv, (argc + 2) * sizeof *argv);
	argv[argc++] = parse_arg(anchor);

	anchor = p;
#undef argv
#undef argc
#undef anchor
}

void parse_cmd(char *cmd, int *pargc, char ***pargv)
{
	int argc = *pargc;
	char **argv = *pargv;

	char *anchor, *p;
	p = anchor = cmd;
	while(*p){
		if(*p == '\\'){
			p++;
			continue;
		}
		if(!isspace(*p)){
			p++;
			continue;
		}

		*p++ = '\0';

		add_argv(&argv, &argc, &anchor, p);
	}

	if(anchor != p)
		add_argv(&argv, &argc, &anchor, p);

	*pargv = argv;
	*pargc = argc;
}

void filter_cmd(int *pargc, char ***pargv)
{
	/* check for '%' */
	int argc = *pargc;
	char **argv = *pargv;
	int i;
	char *fnam = (char *)buffer_fname(buffers_cur());


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

						new = join("", (char *[]){
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

