#ifndef CMDS_H
#define CMDS_H

typedef struct Cmd
{
	const char *cmd;
	void (*func)(void);
} Cmd;

typedef void cmd_func(void);

cmd_func c_q, c_w, c_vs;

#endif
