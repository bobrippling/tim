#ifndef CMDS_H
#define CMDS_H

typedef void cmd_func(int argc, char **argv);

typedef struct cmd_t
{
	const char *cmd;
	cmd_func *func;
} cmd_t;

cmd_func c_q, c_w, c_vs, c_sp, c_e, c_run;

#endif

