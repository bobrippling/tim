#ifndef CMDS_H
#define CMDS_H

#include <stdbool.h>

typedef bool cmd_f_argv(
		int argc, char **argv,
		bool force, struct range *);

typedef bool cmd_f_arg1(
		char *arg,
		bool force, struct range *);

typedef struct cmd_t
{
	const char *cmd;
	union
	{
		cmd_f_argv *f_argv;
		cmd_f_arg1 *f_arg1;
	};
	bool single_arg;
} cmd_t;

cmd_func c_q, c_cq;
cmd_func c_w;
cmd_func c_x;
cmd_func c_e;
cmd_func c_p;
cmd_func c_g;

cmd_func c_vs;
cmd_func c_sp;

cmd_func c_run;

#endif

