#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "prompt.h"

#include "ncurses.h"
#include "io.h"
#include "mem.h"

char *prompt(char promp, const char *initial)
{
	int len;
	char *buf;
	int i;

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

	for(int x = 0; x < i; x++)
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
