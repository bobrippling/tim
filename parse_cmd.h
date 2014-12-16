#ifndef PARSE_CMD_H
#define PARSE_CMD_H

void parse_cmd(char *cmd, int *pargc, char ***pargv, bool shellglob);

bool parse_ranged_cmd(
		char *cmd,
		const cmd_t **pcmd_f,
		char ***pargv, int *pargc,
		bool *pforce,
		struct range **prange);

void free_argv(char **argv, int argc);

#endif
