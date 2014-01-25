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

static void insert_argv(char ***pargv, int *pargc, int idx, char *new)
{
#define argv (*pargv)
#define argc (*pargc)
	argv = urealloc(argv, (++argc + 1) * sizeof *argv);
	for(int i = argc - 1; i > idx; i--)
		argv[i] = argv[i - 1];

	argv[idx] = new;
#undef argv
#undef argc
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
	bool had_punct;

	/* special case */
	if((had_punct = ispunct(cmd[0]))){
		argv = urealloc(argv, (argc + 2) * sizeof *argv);

		snprintf(
				argv[argc++] = umalloc(2),
				2, "%s", cmd);

		cmd++;
	}

	for(char *p = strtok(cmd, " "); p; p = strtok(NULL, " ")){
		argv = urealloc(argv, (argc + 2) * sizeof *argv);
		argv[argc++] = parse_arg(p);
	}
	if(argv)
		argv[argc] = NULL;

	/* special case: check for non-ascii in the first cmd */
	if(!had_punct && argc >= 1){
		for(char *p = argv[0]; *p; p++){
			if(!isalpha(*p)){
				if(*p == '!'){
					*force = true;
					*p = '\0';

					if(p[1]){
						/* split, e.g. "w!hello" -> "w", "hello" */
						insert_argv(&argv, &argc, 1, ustrdup(p + 1));
					}
				}else{
					/* split, e.g. "g/hi/" -> "g", "/hi/" */
					insert_argv(&argv, &argc, 1, ustrdup(p));
					*p = '\0';
				}
				break;
			}
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

