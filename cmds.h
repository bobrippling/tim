#ifndef CMDS_H
#define CMDS_H

#include <stdbool.h>

typedef bool cmd_f_argv(
		int argc, char **argv,
		bool force, struct range *);

typedef bool cmd_f_arg1(
		char *cmd, char *arg,
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

cmd_f_argv c_q, c_cq;
cmd_f_argv c_w;
cmd_f_argv c_x;
cmd_f_argv c_e;
cmd_f_argv c_p;
cmd_f_argv c_j;
cmd_f_argv c_d;

cmd_f_arg1 c_g;
cmd_f_arg1 c_norm;

cmd_f_argv c_vs;
cmd_f_argv c_sp;

cmd_f_arg1 c_run;

bool cmd_dispatch(
		const cmd_t *cmd_f,
		int argc, char **argv,
		bool force, struct range *range);

#endif
