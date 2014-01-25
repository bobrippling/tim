#ifndef PARSE_CMD_H
#define PARSE_CMD_H

bool parse_cmd(
		char *cmd,
		int *pargc, char ***pargv,
		bool *force,
		struct range **region);

void filter_cmd(int *pargc, char ***pargv);

#endif
