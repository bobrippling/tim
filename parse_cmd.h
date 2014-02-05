#ifndef PARSE_CMD_H
#define PARSE_CMD_H

void parse_cmd(char *cmd, int *pargc, char ***pargv);

void filter_cmd(int *pargc, char ***pargv);

#endif
