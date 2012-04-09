#include <stdio.h>

#include "cmds.h"
#include "ui.h"

#include "list.h"
#include "buffer.h"
#include "buffers.h"

void c_q(void)
{
	ui_running = 0;
}

void c_w(void)
{
	FILE *f = fopen("out", "w");
	if(f){
		list_t *l;
		for(l = buffers_cur()->head; l; l = l->next){
			fwrite(l->line, 1, l->len_line, f);
			fputc('\n', f);
		}
		fclose(f);

		ui_status("written to \"out\"");
	}
}
