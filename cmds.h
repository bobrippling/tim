#ifndef CMDS_H
#define CMDS_H

typedef void cmd_func(int argc, char **argv);

typedef struct Cmd
{
	const char *cmd;
	cmd_func *func;
} Cmd;

cmd_func c_q, c_w, c_vs, c_sp, c_e;

#endif

