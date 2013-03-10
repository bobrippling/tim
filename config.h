#ifndef CONFIG_H
#define CONFIG_H

#define KEY_ARG_NONE        { 0 }
#define MOTION_ARG_NONE     { 0 }

const motionkey_t motion_keys[] = {
	{ 'I',            { m_sol,  MOTION_ARG_NONE } },

	{ 'a',            { m_move, { .pos = { 1, 0 } } } },

	/* A -> eol, then move right by 1 */
	{ 'A',            { m_eol,  MOTION_ARG_NONE } },
	{ 'A',            { m_move, { .pos = { 1, 0 } } } },

	{ 'j',            { m_move, { .pos = {  0,  1 } } } },
	{ 'k',            { m_move, { .pos = {  0, -1 } } } },
	{ 'h',            { m_move, { .pos = { -1,  0 } } } },
	{ 'l',            { m_move, { .pos = {  1,  0 } } } },

	{ '0',            { m_goto, { .pos = { 0, -1 } } } },
	{ '^',            { m_sol,  MOTION_ARG_NONE } },
	{ '$',            { m_eol,  MOTION_ARG_NONE } },

	{ 'g',            { m_sof,  MOTION_ARG_NONE } },
	{ 'G',            { m_eof,  MOTION_ARG_NONE } },

	{ 'H',            { m_sos,  MOTION_ARG_NONE } },
	{ 'M',            { m_mos,  MOTION_ARG_NONE } },
	{ 'L',            { m_eos,  MOTION_ARG_NONE } },

	{ 0, { 0, MOTION_ARG_NONE }, }
};

#define INS_KEY(k) \
	{ k, k_set_mode, { UI_INSERT }, UI_NORMAL }

const ikey_t keys[] = {
	/* order is important */
	/* char, func, arg, mode */
	{ '\033',         k_motion,     { .motion = { m_move, .arg.pos = { -1, 0 }}}, UI_INSERT },
	{ '\033',         k_set_mode,   { UI_NORMAL },           UI_NORMAL | UI_INSERT },

	{ 'o',            k_open,       {  1 },                   UI_NORMAL },
	{ 'O',            k_open,       { -1 },                   UI_NORMAL },

	{ 'd',            k_del,        { LINEWISE },  UI_NORMAL },

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

const cmd_t cmds[] = {
	{ "q",   c_q     },
	{ "w",   c_w     },
	{ "e",   c_e     },

	{ "x",   c_x     },

	{ "vs",  c_vs    },
	{ "sp",  c_sp    },

	{ "!",   c_run   },

	{ NULL, NULL },
};

#endif

