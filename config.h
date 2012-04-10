#ifndef CONFIG_H
#define CONFIG_H

#define KEYARG_NONE { 0 }

Key keys[] = {
	/* order is important */

	{ '\033',         k_move,       { .pos = { -1, 0 } },              UI_INSERT },
	{ '\033',         k_set_mode,   { UI_NORMAL },                     UI_NORMAL | UI_INSERT },

	{ 'i',            k_set_mode,   { UI_INSERT },                     UI_NORMAL },

	{ 'a',            k_move,       { .pos = { 1, 0 } },               UI_NORMAL },
	{ 'a',            k_set_mode,   { UI_INSERT },                     UI_NORMAL },

	{ ':',            k_cmd,        KEYARG_NONE,                       UI_NORMAL },

	{ 'j',            k_move,       { .pos = {  0,  1 } },             UI_NORMAL },
	{ 'k',            k_move,       { .pos = {  0, -1 } },             UI_NORMAL },
	{ 'h',            k_move,       { .pos = { -1,  0 } },             UI_NORMAL },
	{ 'l',            k_move,       { .pos = {  1,  0 } },             UI_NORMAL },

	{ '^',            k_sol,        KEYARG_NONE,                       UI_NORMAL },
	{ '$',            k_eol,        KEYARG_NONE,                       UI_NORMAL },

	{ 'g',            k_sof,        KEYARG_NONE,                       UI_NORMAL },
	{ 'G',            k_eof,        KEYARG_NONE,                       UI_NORMAL },

	{ CTRL_AND('l'),  k_redraw,     KEYARG_NONE,                       UI_NORMAL },


	{ 0, NULL, KEYARG_NONE, UI_NORMAL }
};

Cmd cmds[] = {
	{ "q", c_q },
	{ "w", c_w },
	{ NULL, NULL },
};

#endif
