#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "prompt.h"

#include "ncurses.h"
#include "io.h"
#include "mem.h"

#include "parse_cmd.h"

static void expand_filename(
		int *const pi,
		char **const pbuf,
		int *const plen,
		enum prompt_expansion expand_mode)
{
	char *buf = *pbuf;
	buf[*pi] = '\0';

	int argc = 0;
	char **argv = NULL;

	/* TODO: add '*' on the end of buf */
	parse_cmd(buf, &argc, &argv, true);

	char *joined = join(" ", argv, argc);

	free_argv(argv, argc);

	free(*pbuf);
	*pbuf = joined;
	*pi = *plen = strlen(joined);
}

static void expand_exec(
		int *const pi,
		char **const pbuf,
		int *const plen,
		enum prompt_expansion expand_mode)
{
	/* TODO */
}

static bool expand(
		int *const pi,
		char **const pbuf,
		int *const plen,
		enum prompt_expansion expand_mode)
{
	switch(expand_mode){
		case PROMPT_NONE:
			return false;

		case PROMPT_EXEC:
		{
			/* very simple - no space, do something from $PATH.
			 * otherwise fall through to filename */
			char *buf = *pbuf;

			for(int i = 0; i < *pi; i++){
				if(isspace(buf[i])){
					expand_filename(pi, pbuf, plen, expand_mode);
					return true;
				}
			}

			expand_exec(pi, pbuf, plen, expand_mode);
			break;
		}

		case PROMPT_FILENAME:
			expand_filename(pi, pbuf, plen, expand_mode);
			break;
	}

	return true;
}

char *prompt(char promp, enum prompt_expansion expand_mode)
{
	int reading = 1;
	int len = 10;
	char *buf = umalloc(len);
	int y, x;
	int i = 0;

	nc_get_yx(&y, &x);

	nc_set_yx(nc_LINES() - 1, 0);
	nc_addch(promp);
	nc_clrtoeol();

	while(reading){
		bool wasraw;
		int ch = io_getch(IO_NOMAP | IO_MAPRAW, &wasraw, true);

		switch(ch){
			case CTRL_AND('?'):
			case CTRL_AND('H'):
			case 263:
			case 127:
				/* backspace */
				if(i == 0)
					goto cancel;
				buf[--i] = '\0';
				nc_set_yx(nc_LINES() - 1, i + 1);
				break;

			case '\n':
			case '\r':
				reading = 0;
				break;

			case '\t':
				if(expand(&i, &buf, &len, expand_mode)){
					/* redraw */
					nc_set_yx(nc_LINES() - 1, 0);
					nc_addch(promp);
					nc_addstr(buf);
					nc_clrtoeol();
				}else{
					goto def;
				}
				break;

			case CTRL_AND('u'):
				buf[i = 0] = '\0';
				nc_set_yx(nc_LINES() - 1, i + 1);
				break;

			case K_ESC:
				if(!wasraw)
					goto cancel;
				/* fall */

def:
			default:
				buf[i++] = ch;
				nc_addch(ch);
				if(i == len)
					buf = urealloc(buf, len *= 2);
				buf[i] = '\0';
				break;
		}
	}

	nc_set_yx(y, x);
	return buf;
cancel:
	nc_set_yx(y, x);
	free(buf);
	return NULL;
}
