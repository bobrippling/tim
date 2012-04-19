#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mem.h"
#include "ui.h"

int shellout(const char *cmd)
{
	const char *shcmd;
	int r;

	if(cmd){
		shcmd = cmd;
	}else{
		const char *shell = getenv("SHELL");
		if(!shell)
			shell = "sh";
		shcmd = shell;
	}

	ui_term();

	r = system(shcmd);

	fprintf(stderr, "enter to continue...");
	for(;;){
		int c = getchar();
		if(c == EOF || c == '\n')
			break;
	}

	ui_init();

	return r;
}
