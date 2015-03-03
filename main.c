#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "ui.h"
#include "window.h"
#include "windows.h"

static char **remaining_fnames;

char *args_next_fname(bool pop)
{
	char *fname = remaining_fnames ? *remaining_fnames : NULL;
	if(pop && fname)
		remaining_fnames++;
	return fname;
}

int main(int argc, char **argv)
{
	int i;
	unsigned offset = 0;
	enum windows_init_args initargs = WIN_NONE;

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-O")){
			initargs = WIN_VALL;
		}else if(!strcmp(argv[i], "-o")){
			initargs = WIN_HALL;
		}else if(*argv[i] == '+'){
			char *p = argv[i] + 1;

			while(isdigit(*p))
				p++;

			if(*p == '\0' && sscanf(argv[i]+1, "%u", &offset) == 1){
				/* continue */
				if(offset > 0)
					offset--; /* 1-base -> 0-base */
			}else{
				break;
			}
		}else{
			break;
		}
	}

	ui_init(); /* must be before buffers_init() */

	windows_init(argc - i, argv + i, initargs, offset);

	int r = ui_main();
	ui_term();

	windows_term();

	return r;
}
