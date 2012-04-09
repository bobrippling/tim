#include <stdio.h>

#include "ui.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"

int main(int argc, char **argv)
{
	buffers_init(argc - 1, argv + 1);

	ui_init();
	ui_main();
	ui_term();

	return 0;
}
