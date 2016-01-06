#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"
#include "window.h"
#include "windows.h"
#include "tab.h"
#include "tabs.h"

buffer_t *buffers_find(const char *fname)
{
	if(!windows_cur()) /* called on startup */
		return NULL;

	for(tab *tab = tabs_first(); tab; tab = tab->next){
		window *win;
		ITER_WINDOWS_FROM(win, tab->win){
			buffer_t *buf = win->buf;
			const char *thisfname = buffer_fname(buf);

			if(thisfname && !strcmp(thisfname, fname))
				return buf;
		}
	}

	return NULL;
}

bool buffers_modified_single(const buffer_t *b)
{
	return b->modified && buffer_opencount(b) == 1;
}

bool buffers_modified_excluding(buffer_t *excluding)
{
	for(tab *tab = tabs_first(); tab; tab = tab->next){
		window *win;
		ITER_WINDOWS_FROM(win, tab->win){
			buffer_t *buf = win->buf;
			if(buf != excluding && buf->modified)
				return true;
		}
	}

	return false;
}
