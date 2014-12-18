#ifndef CONFIG_CMDS_H
#define CONFIG_CMDS_H

const cmd_t cmds[] = {
	{ "q",   .f_argv = c_q   },
	{ "cq",  .f_argv = c_cq  },
	{ "qa",  .f_argv = c_qa  },
	{ "w",   .f_argv = c_w   },
	{ "wa",  .f_argv = c_wa  },
	{ "e",   .f_argv = c_e   },
	{ "r",   .f_arg1 = c_r, .single_arg = true },
	{ "ene", .f_argv = c_ene },
	{ "on",  .f_argv = c_only },

	{ "x",   .f_argv = c_x   },
	{ "xa",  .f_argv = c_xa  },
	{ "wq",  .f_argv = c_x   },

	{ "vs",  .f_argv = c_vs },
	{ "vnew", .f_argv = c_vnew },
	{ "sp",  .f_argv = c_sp },
	{ "new", .f_argv = c_new },

	{ "n", .f_argv = c_n },

	{ "p",   .f_argv = c_p   },
	{ "j",   .f_argv = c_j   },
	{ "d",   .f_argv = c_d   },
	{ "m",   .f_argv = c_m   },

	{ "g",   .f_arg1 = c_g, .single_arg = true, .skipglob = true },
	{ "v",   .f_arg1 = c_g, .single_arg = true, .inverse = true, .skipglob = true },

	{ "!",   .f_arg1 = c_run, .single_arg = true, .skipglob = true },

	{ "norm", .f_arg1 = c_norm, .single_arg = true, .skipglob = true },

	{ NULL }
};

#endif
