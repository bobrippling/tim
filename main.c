#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ui.h"
#include "pos.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"

int main(int argc, char **argv)
{
	int i;
	unsigned offset = 0;
	enum buffer_init_args initargs = BUF_NONE;

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-O")){
			initargs = BUF_VALL;
		}else if(!strcmp(argv[i], "-o")){
			initargs = BUF_HALL;
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

	ui_init();

	buffers_init(argc - i, argv + i, initargs, offset);

	ui_main();
	ui_term();

	return 0;
}
