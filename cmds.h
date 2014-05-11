#ifndef CMDS_H
#define CMDS_H

#include <stdbool.h>

typedef bool cmd_func(
		int argc, char **argv, bool force)
	__attribute__((nonnull));

typedef struct cmd_t
{
	const char *cmd;
	cmd_func *func;
} cmd_t;

cmd_func c_q, c_cq;
cmd_func c_w;
cmd_func c_x;
cmd_func c_e;

cmd_func c_y;

cmd_func c_vs;
cmd_func c_sp;

cmd_func c_run;

#endif

