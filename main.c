#include <stdio.h>

#include "ui.h"

int main(int argc, char **argv)
{
	if(argc != 1){
		fprintf(stderr, "Usage: %s\n", *argv);
		return 1;
	}

	ui_init();
	ui_main();
	ui_term();

	return 0;
}
