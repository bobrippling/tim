#ifndef CONFIG_H
#define CONFIG_H

#define KEY_ARG_NONE    { 0 }
#define MOTION_ARG_NONE { 0 }

MotionKey motion_keys[] = {
	{ '\033',         m_move,       { .pos = { -1, 0 } },              UI_INSERT },

	{ 'a',            m_move,       { .pos = { 1, 0 } },               UI_NORMAL },

	{ 'j',            m_move,       { .pos = {  0,  1 } },             UI_NORMAL },
	{ 'k',            m_move,       { .pos = {  0, -1 } },             UI_NORMAL },
	{ 'h',            m_move,       { .pos = { -1,  0 } },             UI_NORMAL },
	{ 'l',            m_move,       { .pos = {  1,  0 } },             UI_NORMAL },

	{ '0',            m_goto,       { .pos = { 0, -1 } },              UI_NORMAL },
	{ '^',            m_sol,        MOTION_ARG_NONE,                   UI_NORMAL },
	{ '$',            m_eol,        MOTION_ARG_NONE,                   UI_NORMAL },

	{ 'g',            m_sof,        MOTION_ARG_NONE,                   UI_NORMAL },
	{ 'G',            m_eof,        MOTION_ARG_NONE,                   UI_NORMAL },

	{ 'H',            m_sos,        MOTION_ARG_NONE,                   UI_NORMAL },
	{ 'M',            m_mos,        MOTION_ARG_NONE,                   UI_NORMAL },
	{ 'L',            m_eos,        MOTION_ARG_NONE,                   UI_NORMAL },

	{ 0, NULL, MOTION_ARG_NONE, 0 }
};

Key keys[] = {
	/* order is important */
	{ '\033',         k_set_mode,   { UI_NORMAL },           UI_NORMAL | UI_INSERT },
	{ 'i',            k_set_mode,   { UI_INSERT },           UI_NORMAL },
	{ 'a',            k_set_mode,   { UI_INSERT },           UI_NORMAL },

	{ ':',            k_cmd,        KEY_ARG_NONE,            UI_NORMAL }, /* k_set_mode instead? */

	{ CTRL_AND('l'),  k_redraw,     KEY_ARG_NONE,            UI_NORMAL },

	{ CTRL_AND('e'),  k_scroll,     { .pos = { 0,  2 } },     UI_NORMAL },
	{ CTRL_AND('y'),  k_scroll,     { .pos = { 0, -2 } },     UI_NORMAL },


	{ 0, NULL, KEY_ARG_NONE, UI_NORMAL }
};

Cmd cmds[] = {
	{ "q", c_q },
	{ "w", c_w },
	{ NULL, NULL },
};

#endif
