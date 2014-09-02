#ifndef CONFIG_CMDS_H
#define CONFIG_CMDS_H

const cmd_t cmds[] = {
	{ "q",   .f_argv = c_q   },
	{ "cq",  .f_argv = c_cq  },
	{ "qa",  .f_argv = c_qa  },
	{ "w",   .f_argv = c_w   },
	{ "e",   .f_argv = c_e   },
	{ "r",   .f_arg1 = c_r, .single_arg = true },

	{ "x",   .f_argv = c_x   },
	{ "wq",  .f_argv = c_x   },

	{ "vs",  .f_argv = c_vs  },
	{ "sp",  .f_argv = c_sp  },

	{ "p",   .f_argv = c_p   },
	{ "j",   .f_argv = c_j   },
	{ "d",   .f_argv = c_d   },
	{ "m",   .f_argv = c_m   },

	{ "g",   .f_arg1 = c_g, .single_arg = true },
	{ "v",   .f_arg1 = c_g, .single_arg = true, .inverse = true },

	{ "!",   .f_arg1 = c_run, .single_arg = true },

	{ "norm", .f_arg1 = c_norm, .single_arg = true },

	{ NULL }
};

#endif
