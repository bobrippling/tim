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
#include "window.h"
#include "windows.h"

#include "parse_cmd.h"

#include "config_cmds.h"

static void add_argv(
		char ***pargv, int *pargc,
		char **panchor, char *current,
		bool shellglob)
{
#define argv (*pargv)
#define argc (*pargc)
#define anchor (*panchor)
	char *failbuf;

	wordexp_t wexp = { 0 };
	int argc_inc;
	char **argv_add;

	if(shellglob && 0 == wordexp(anchor, &wexp, WRDE_NOCMD)){
		argc_inc = wexp.we_wordc;
		argv_add = wexp.we_wordv;
	}else{
		argc_inc = 1;
		argv_add = &failbuf;
		failbuf = anchor;
	}

	argv = urealloc(argv, (argc + argc_inc + 1) * sizeof *argv);
	for(int i = 0; i < argc_inc; i++)
		argv[argc + i] = ustrdup(argv_add[i]);

	argc += argc_inc;

	wordfree(&wexp);

	anchor = current;
#undef argv
#undef argc
#undef anchor
}

void parse_cmd(char *cmd, int *pargc, char ***pargv, bool shellglob)
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

		add_argv(&argv, &argc, &anchor, p, shellglob);
	}

	if(anchor != p)
		add_argv(&argv, &argc, &anchor, p, shellglob);

	*pargv = argv;
	*pargc = argc;
}

static void filter_argv(int *pargc, char ***pargv)
{
	/* check for '%', '#' and * wildcard expansion */
	int argc = *pargc;
	char **argv = *pargv;
	int i;
	const char *fnam = buffer_fname(buffers_cur());

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
								(char *)fnam,
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

	/* alpha or single non-ascii for command */
	argc = 1;
	argv = umalloc((argc + 1) * sizeof *argv);

	if(isalpha(*cmd_i)){
		char *p;
		for(p = cmd_i + 1; isalpha(*p); p++);
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
	for(int i = 0; cmds[i].arg0; i++)
		if(!strcmp(cmds[i].arg0, argv[0])){
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
		parse_cmd(cmd_i, &argc, &argv, !cmd_f->skipglob);
	}
	argv[argc] = NULL;

	filter_argv(&argc, &argv);

	return true;
#undef cmd_f
#undef argv
#undef argc
#undef force
#undef range
}

void free_argv(char **argv, int argc)
{
	for(int i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
}
