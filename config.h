#ifndef CONFIG_H
#define CONFIG_H

#define KEYARG_NONE { 0 }

Key keys[] = {
	/* order is important */

	{ ':',            k_cmd,        KEYARG_NONE,      UI_NORMAL },

	{ '\033',         k_left,       KEYARG_NONE,      UI_INSERT },
	{ '\033',         k_set_mode,   { UI_NORMAL },    UI_NORMAL | UI_INSERT },
	{ 'i',            k_set_mode,   { UI_INSERT },    UI_NORMAL },

	{ 'j',            k_down,       KEYARG_NONE,      UI_NORMAL },
	{ 'k',            k_up,         KEYARG_NONE,      UI_NORMAL },
	{ 'h',            k_left,       KEYARG_NONE,      UI_NORMAL },
	{ 'l',            k_right,      KEYARG_NONE,      UI_NORMAL },

	{ CTRL_AND('l'),  k_redraw,     KEYARG_NONE,      UI_NORMAL },

	{ 0, NULL, KEYARG_NONE, UI_NORMAL }
};

Cmd cmds[] = {
	{ "q", c_q },
	{ "w", c_w },
	{ NULL, NULL },
};

#endif
