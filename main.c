#include <stdio.h>

#include "ui.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"

int main(int argc, char **argv)
{
	ui_init();

	buffers_init(argc - 1, argv + 1);

	ui_main();
	ui_term();

	return 0;
}
