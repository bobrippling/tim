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
#include "cmds.h"

#include "parse_cmd.h"

#include "config_cmds.h"

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

bool parse_ranged_cmd(
		char *cmd,
		const cmd_t **pcmd_f,
		char ***pargv, int *pargc,
		bool *pforce,
		struct range **prange)
{
#define cmd_f (*pcmd_f)
#define argv (*pargv)
#define argc (*pargc)
#define force (*pforce)
#define range (*prange)
	argv = NULL;
	argc = 0;

	char *cmd_i = cmd;
	while(isspace(*cmd_i))
		cmd_i++;

	if(!*cmd_i)
		return false;

	switch(parse_range(cmd_i, &cmd_i, range)){
		case RANGE_PARSE_FAIL:
			return false;
		case RANGE_PARSE_NONE:
			range = NULL;
		case RANGE_PARSE_PASS:
			break;
	}

	/* alnum or single non-ascii for command */
	argc = 1;
	argv = umalloc((argc + 1) * sizeof *argv);

	if(isalnum(*cmd_i)){
		char *p;
		for(p = cmd_i + 1; isalnum(*p); p++);
		argv[0] = ustrdup_len(cmd_i, p - cmd_i);
		cmd_i = p - 1;
	}else if(*cmd_i){
		argv[0] = ustrdup_len(cmd_i, 1);
	}else{
		return false;
	}
	cmd_i++;

	/* look for command */
	cmd_f = NULL;
	for(int i = 0; cmds[i].cmd; i++)
		if(!strcmp(cmds[i].cmd, argv[0])){
			cmd_f = &cmds[i];
			break;
		}

	if(!cmd_f)
		return false;

	force = false;
	for(; isspace(*cmd_i); cmd_i++);
	if(*cmd_i == '!')
		force = true, cmd_i++;

	if(cmd_f->single_arg){
		argv = urealloc(argv, (++argc + 1) * sizeof *argv);
		argv[1] = ustrdup(cmd_i);

	}else{
		parse_cmd(cmd_i, &argc, &argv);
	}
	argv[argc] = NULL;

	filter_cmd(&argc, &argv);

	return true;
#undef cmd_f
#undef argv
#undef argc
#undef force
#undef range
}
