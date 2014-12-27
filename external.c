#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ncurses.h>

#include <termios.h>
#include <unistd.h>

#include "mem.h"
#include "ui.h"
#include "external.h"

#ifdef FANCY_TERM
static int term_canon(int on)
{
	struct termios this;

	if(tcgetattr(0, &this))
		return 0;

	if(on)
		this.c_lflag |=  ICANON;
	else
		this.c_lflag &= ~ICANON;

	return 0 == tcsetattr(0, 0, &this);
}
#endif

static void shellout_wait_ret(void)
{
#ifdef FANCY_TERM
	if(term_canon(0)){
		int unget;

		fprintf(stderr, "any key to continue...");
		/* use getch() so we don't go behind ncurses' back */
		while((unget = getch()) == -1);
		term_canon(1);
		fputc('\n', stderr);
		ungetch(unget);
	}else{
#endif
		fprintf(stderr, "enter to continue...");
		for(;;){
			int c = getchar();
			if(c == EOF || c == '\n')
				break;
		}
#ifdef FANCY_TERM
	}
#endif
}

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

	shellout_wait_ret();

	ui_init();

	return r;
}
