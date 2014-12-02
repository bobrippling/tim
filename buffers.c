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

buffer_t *buffers_find(const char *fname)
{
	if(!windows_cur()) /* called on startup */
		return NULL;

	window *w;
	ITER_WINDOWS(w){
		buffer_t *b = w->buf;
		const char *thisfname = buffer_fname(b);

		if(thisfname && !strcmp(thisfname, fname))
			return b;
	}

	return NULL;
}

bool buffers_modified_single(const buffer_t *b)
{
	return b->modified && buffer_opencount(b) == 1;
}
