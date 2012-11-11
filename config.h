#ifndef CONFIG_H
#define CONFIG_H

#define KEY_ARG_NONE    { 0 }
#define MOTION_ARG_NONE { 0 }

motionkey_t motion_keys[] = {
	{ '\033',         m_move,       { .pos = { -1, 0 } },              UI_INSERT },

	{ 'I',            m_sol,        MOTION_ARG_NONE,                   UI_NORMAL },

	{ 'a',            m_move,       { .pos = { 1, 0 } },               UI_NORMAL },

	{ 'A',            m_eol,        { .pos = { 1, 0 } },               UI_NORMAL },
	{ 'A',            m_move,       { .pos = { 1, 0 } },               UI_NORMAL },

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

	{ 0 }
};

#define INS_KEY(k) \
	{ k, k_set_mode, { UI_INSERT }, UI_NORMAL }

key_t keys[] = {
	/* order is important */
	{ '\033',         k_set_mode,   { UI_NORMAL },           UI_NORMAL | UI_INSERT },

	{ 'o',            k_open,       {  1 },                   UI_NORMAL },
	{ 'O',            k_open,       { -1 },                   UI_NORMAL },

	INS_KEY('i'),
	INS_KEY('I'),
	INS_KEY('a'),
	INS_KEY('A'),
	INS_KEY('o'),
	INS_KEY('O'),

	{ ':',            k_cmd,        KEY_ARG_NONE,            UI_NORMAL }, /* k_set_mode instead? */

	{ CTRL_AND('l'),  k_redraw,     KEY_ARG_NONE,            UI_NORMAL },

	{ CTRL_AND('e'),  k_scroll,     { .pos = { 0,  2 } },     UI_NORMAL },
	{ CTRL_AND('y'),  k_scroll,     { .pos = { 0, -2 } },     UI_NORMAL },

	{ CTRL_AND('w'),  k_winsel,     KEY_ARG_NONE,             UI_NORMAL },

	{ CTRL_AND('g'),  k_show,       KEY_ARG_NONE,             UI_NORMAL },


	{ 0, NULL, KEY_ARG_NONE, UI_NORMAL }
};

cmd_t cmds[] = {
	{ "q",   c_q     },
	{ "w",   c_w     },
	{ "e",   c_e     },

	{ "vs",  c_vs    },
	{ "sp",  c_sp    },

	{ "!",   c_run   },

	{ NULL, NULL },
};

#endif

