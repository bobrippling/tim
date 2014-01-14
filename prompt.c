#include <stdlib.h>
#include <stdarg.h>

#include "prompt.h"

#include "ncurses.h"
#include "io.h"
#include "mem.h"
#include "keymacros.h"

char *prompt(char promp)
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
		int ch = io_getch(IO_NOMAP | IO_MAPRAW, &wasraw);

		switch(ch){
			case_BACKSPACE:
				if(i == 0)
					goto cancel;
				buf[--i] = '\0';
				nc_set_yx(nc_LINES() - 1, i + 1);
				break;

			case '\n':
			case '\r':
				reading = 0;
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
