#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ncurses.h>

#include <termios.h>
#include <unistd.h>

#include "mem.h"
#include "ui.h"

#include "pos.h"
#include "region.h"
#include "list.h"

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

void shellout_wait_ret(void)
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

bool shellout_write(const char *cmd, list_t *seeked, int nlines)
{
	ui_term();

	FILE *subp = popen(cmd, "w");

	if(!subp){
		fprintf(stderr, "fork/exec: %s", strerror(errno));
		return false;
	}

	int r = list_write_file(seeked, nlines, subp, /*eol*/true);
	int errno_write = errno;

	int waitret = pclose(subp);
	int errno_wait = errno;

	if(r){
		fprintf(stderr, "write subprocess: %s", strerror(errno_write));
		return false;
	}

	if(WIFEXITED(waitret)){
		int ec = WEXITSTATUS(waitret);

		if(ec)
			fprintf(stderr, "command returned %d\n", ec);

	}else if(WIFSIGNALED(waitret)){
		fprintf(stderr, "command signalled with %d\n", WTERMSIG(waitret));
	}else if(WIFSTOPPED(waitret)){
		fprintf(stderr, "command stopped with %d\n", WSTOPSIG(waitret));
	}else{
		fprintf(stderr, "unknown state from wait()/pclose(): %d: %s",
				waitret, strerror(errno_wait));
	}

	shellout_wait_ret();

	return true;
}
