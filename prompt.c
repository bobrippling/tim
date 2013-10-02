#include <stdlib.h>
#include <stdarg.h>

#include "prompt.h"

#include "ncurses.h"
#include "io.h"
#include "mem.h"

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
		int ch = io_getch(IO_NOMAP);

		switch(ch){
			case '\033':
				goto cancel;

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

			case CTRL_AND('u'):
				buf[i = 0] = '\0';
				nc_set_yx(nc_LINES() - 1, i + 1);
				break;

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
