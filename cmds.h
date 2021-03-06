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
	const char *arg0;
	union
	{
		cmd_f_argv *f_argv;
		cmd_f_arg1 *f_arg1;
	};
	bool single_arg;
	bool inverse; /* e.g. :v */
	bool skipglob;
} cmd_t;

cmd_f_argv c_q, c_cq, c_qa;
cmd_f_argv c_wa;
cmd_f_argv c_x, c_xa;
cmd_f_argv c_e, c_ene;
cmd_f_argv c_only;
cmd_f_argv c_p;
cmd_f_argv c_j;
cmd_f_argv c_d;
cmd_f_argv c_m, c_t;

cmd_f_argv c_n;

cmd_f_arg1 c_g;
cmd_f_arg1 c_norm;
cmd_f_arg1 c_r, c_w;

cmd_f_argv c_vs, c_vnew;
cmd_f_argv c_sp, c_new;
cmd_f_argv c_all;

cmd_f_argv c_tabe, c_tabo;

cmd_f_arg1 c_run;

cmd_f_argv c_ls;

bool cmd_dispatch(
		const cmd_t *cmd_f,
		int argc, char **argv,
		bool force, struct range *range);

bool edit_common(const char *fname, bool const force);

#endif
