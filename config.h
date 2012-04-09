#ifndef CONFIG_H
#define CONFIG_H

Key keys[] = {
	{ ':', k_cmd  },
	{ 'i', k_ins  },

	{ 'j', k_down   },
	{ 'k', k_up     },
	{ 'h', k_left   },
	{ 'l', k_right  },

	{ CTRL_AND('l'), k_redraw },

	{ 0,   NULL },
};

Cmd cmds[] = {
	{ "q", c_q },
	{ "w", c_w },
	{ NULL, NULL },
};

#endif
