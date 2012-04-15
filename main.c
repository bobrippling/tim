#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "list.h"
#include "pos.h"
#include "buffer.h"
#include "buffers.h"

int main(int argc, char **argv)
{
	int i;
	enum buffer_init_args initargs;

	initargs = 0;

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-O")){
			initargs = BUF_VALL;
		}else if(!strcmp(argv[i], "-o")){
			initargs = BUF_HALL;
		}else{
			break;
		}
	}

	ui_init();

	buffers_init(argc - i, argv + i, initargs);

	ui_main();
	ui_term();

	return 0;
}
