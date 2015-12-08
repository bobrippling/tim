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
	bool ret = false;

	ui_term();

	pid_t child = fork();

	if(child == 0){
		if(setsid() != 0)
			fprintf(stderr, "warning: setsid(): %s\n", strerror(errno));

		FILE *subp = popen(cmd, "w");

		if(!subp){
			fprintf(stderr, "fork/exec: %s\n", strerror(errno));
			exit(127);
		}

		const int write_ret = list_write_file(seeked, nlines, subp, /*eol*/true);
		const int errno_write = errno;

		const int waitret = pclose(subp);
		const int errno_wait = errno;

		if(write_ret){
			fprintf(stderr, "write subprocess: %s", strerror(errno_write));
			exit(127);
		}

		if(waitret == -1){
			fprintf(stderr, "wait(): %s\n", strerror(errno_wait));
			exit(127);
		}

		if(WIFEXITED(waitret))
			exit(WEXITSTATUS(waitret));
		exit(127);

	}else if(child == -1){
		fprintf(stderr, "fork: %s\n", strerror(errno));
		goto out;

	}else{
		/* wait on child */
		int status;

		const pid_t wait_status = waitpid(child, &status, 0);
		const int errno_wait = errno;

		if(wait_status != child){
			if(wait_status == -1){
				fprintf(stderr, "waitpid(): %s\n", strerror(errno));
			}else{
				fprintf(stderr,
						"unexpected wait return %d: %s\n",
						wait_status, strerror(errno));
			}

			/* carry on anyway */
			status = 1;
		}

		if(WIFEXITED(status)){
			int ec = WEXITSTATUS(status);

			if(ec)
				fprintf(stderr, "command returned %d\n", ec);

		}else if(WIFSIGNALED(status)){
			fprintf(stderr, "command signalled with %d\n", WTERMSIG(status));
		}else if(WIFSTOPPED(status)){
			fprintf(stderr, "command stopped with %d\n", WSTOPSIG(status));
		}else{
			fprintf(stderr, "unknown state from wait()/pclose(): %d: %s",
					status, strerror(errno_wait));
		}
	}

out:
	shellout_wait_ret();

	return ret;
}
