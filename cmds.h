#ifndef CMDS_H
#define CMDS_H

typedef struct Cmd
{
	const char *cmd;
	void (*func)(void);
} Cmd;

void c_q(void);

#endif
