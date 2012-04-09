#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "buffer.h"
#include "buffers.h"
#include "mem.h"
#include "ui.h"

static buffer_t   *buf_sel;
static char      **buf_list;
static int         buf_c;

buffer_t *buffers_cur()
{
	return buf_sel;
}

void buffers_init(int argc, char **argv)
{
	int i;

	buf_sel = buffer_new();

	if(argc){
		FILE *f;

		buf_c = argc;
		buf_list = umalloc(argc * sizeof *buf_list);
		for(i = 0; i < argc; i++)
			buf_list[i] = ustrdup(argv[i]);

		f = fopen(argv[0], "r");
		if(f){
			char buf[256];
			while(fgets(buf, sizeof buf, f))
				/*ui_instext(buf)*/;
			fclose(f);
		}else{
			ui_status("\"%s\": %s", argv[0], strerror(errno));
		}
	}
}