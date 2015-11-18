#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "prompt.h"

#include "ncurses.h"
#include "io.h"
#include "mem.h"

/* ctrl-r */
#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "window.h"
#include "windows.h"
#include "buffers.h"

static void prompt_ctrl_r(char **const buf, size_t *const len, size_t *const i)
{
	int y, x;
	nc_get_yx(&y, &x);
	nc_addch('"');
	nc_set_yx(y, x);

	bool wasraw;
	int ch = io_getch(IO_NOMAP, &wasraw, /*domaps:*/true);

	/* f: cursor filename
	 * w: cursor word
	 * W: cursor WORD
	 * %: filename [% register]
	 */
	char *to_insert = NULL;
	bool free_to_insert = true;

	switch(ch){
		case CTRL_AND('f'):
			to_insert = window_current_fname(windows_cur());
			break;
		case CTRL_AND('w'):
			to_insert = window_current_word(windows_cur());
			break;
		case '%':
			to_insert = (char *)buffer_fname(buffers_cur());
			free_to_insert = false;
			break;

		default:
			goto cancel;
	}

	if(!to_insert)
		goto cancel;

	size_t to_insert_len = strlen(to_insert);

	if(*i + to_insert_len + 1 >= *len){
		*len = *i + to_insert_len + 1;
		*buf = urealloc(*buf, *len);
	}

	memcpy(*buf + *i, to_insert, to_insert_len + 1);
	*i += to_insert_len;

	if(free_to_insert)
		free(to_insert);
	return;

cancel:
	nc_clrtoeol();
}

char *prompt(char promp, const char *initial)
{
	size_t len;
	char *buf;
	size_t i;

	if(initial){
		len = strlen(initial) + 1;

		buf = umalloc(len);
		i = len - 1;

		strcpy(buf, initial);

	}else{
		len = 10;
		buf = umalloc(len);
		i = 0;
	}

	int y, x;
	nc_get_yx(&y, &x);
	nc_set_yx(nc_LINES() - 1, 0);
	nc_addch(promp);

	for(size_t x = 0; x < i; x++)
		nc_addch(buf[x]);

	nc_clrtoeol();

	bool reading = true;

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
				reading = false;
				break;

			case CTRL_AND('r'):
				prompt_ctrl_r(&buf, &len, &i);
				nc_set_yx(nc_LINES() - 1, 1);
				nc_addstr(buf);
				break;

			case CTRL_AND('u'):
				buf[i = 0] = '\0';
				nc_set_yx(nc_LINES() - 1, i + 1);
				break;

			case K_ESC:
				if(!wasraw)
					goto cancel;
				/* fall */

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

bool yesno(const char *fmt, ...)
{
	int y, x;
	nc_get_yx(&y, &x);
	nc_set_yx(nc_LINES() - 1, 0);

	va_list l;
	va_start(l, fmt);
	nc_vprintf(fmt, l);
	va_end(l);

	bool wasraw;
	int ch = io_getch(IO_NOMAP, &wasraw, /*domaps:*/true);

	nc_set_yx(y, x);

	return tolower(ch) == 'y';
}
