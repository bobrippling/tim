#ifndef CONFIG_CMDS_H
#define CONFIG_CMDS_H

const cmd_t cmds[] = {
	{ "q",   .f_argv = c_q   },
	{ "cq",  .f_argv = c_cq  },
	{ "w",   .f_argv = c_w   },
	{ "e",   .f_argv = c_e   },

	{ "x",   .f_argv = c_x   },
	{ "wq",  .f_argv = c_x   },

	{ "vs",  .f_argv = c_vs  },
	{ "sp",  .f_argv = c_sp  },

	{ "p",   .f_argv = c_p   },
	{ "j",   .f_argv = c_j   },

	{ "g",   .f_arg1 = c_g, .single_arg = true },

	{ "!",   .f_argv = c_run },

	{ NULL }
};

#endif
