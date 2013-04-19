#ifndef CONFIG_H
#define CONFIG_H

#define KEY_ARG_NONE        { 0 }
#define MOTION_ARG_NONE     { 0 }

const motionkey_t motion_keys[] = {
	/* FIXME: insert motions only in insert mode */
	{ 'I',            { m_sol,  MOTION_ARG_NONE, M_LINEWISE } },

	{ 'a',            { m_move, { .pos = { 1, 0 } }, M_LINEWISE } },

	/* A -> eol, then move right by 1 */
	{ 'A',            { m_eol,  MOTION_ARG_NONE, M_LINEWISE } },
	{ 'A',            { m_move, { .pos = { 1, 0 } }, M_LINEWISE } },

	{ 'j',            { m_move, { .pos = {  0,  1 } }, M_LINEWISE } },
	{ 'k',            { m_move, { .pos = {  0, -1 } }, M_LINEWISE } },
	{ 'h',            { m_move, { .pos = { -1,  0 } }, M_EXCLUSIVE } },
	{ 'l',            { m_move, { .pos = {  1,  0 } }, M_EXCLUSIVE } },

	{ '{',            { m_para,     { -1 }, M_LINEWISE } },
	{ '}',            { m_para,     { +1 }, M_LINEWISE } },

	{ 'f',            { m_find,     { .find_type = 0             }, M_NONE } },
	{ 'F',            { m_find,     { .find_type = F_REV         }, M_NONE } },
	{ 't',            { m_find,     { .find_type = F_TIL         }, M_NONE } },
	{ 'T',            { m_find,     { .find_type = F_REV | F_TIL }, M_NONE } },
	{ ';',            { m_findnext, { .find_type = 0             }, M_NONE } },
	{ ',',            { m_findnext, { .find_type = F_REV         }, M_NONE } },

	{ '0',            { m_goto, { .pos = { 0, -1 } }, M_EXCLUSIVE } },
	{ '^',            { m_sol,  MOTION_ARG_NONE,      M_EXCLUSIVE } },
	{ '$',            { m_eol,  MOTION_ARG_NONE,      M_NONE      } },

	{ 'g',            { m_sof,  MOTION_ARG_NONE, M_LINEWISE } }, // FIXME: goto
	{ 'G',            { m_eof,  MOTION_ARG_NONE, M_LINEWISE } },

	{ 'H',            { m_sos,  MOTION_ARG_NONE, M_LINEWISE } },
	{ 'M',            { m_mos,  MOTION_ARG_NONE, M_LINEWISE } },
	{ 'L',            { m_eos,  MOTION_ARG_NONE, M_LINEWISE } },

	{ '/',            { m_search, { 0               }, M_EXCLUSIVE } },
	{ '?',            { m_search, { 1 /* reverse */ }, M_EXCLUSIVE } },

	{ 0, { 0, MOTION_ARG_NONE, M_NONE } }
};

#define INS_KEY(k) \
	{ k, k_set_mode, { UI_INSERT }, UI_NORMAL }

const ikey_t keys[] = {
	/* order is important */
	/* char, func, arg, mode */
	{ '\033',         k_motion,     { .motion = { m_move, .arg.pos = { -1, 0 }}}, UI_INSERT },
	{ '\033',         k_set_mode,   { UI_NORMAL },           UI_INSERT },

	{ 'o',            k_open,       {  1 },                   UI_NORMAL },
	{ 'O',            k_open,       { -1 },                   UI_NORMAL },

	/* TODO: C = c$, D = d$ */
	{ 'd',            k_del,        KEY_ARG_NONE,  UI_NORMAL },
	{ 'c',            k_del,        KEY_ARG_NONE,  UI_NORMAL },
	{ 'c',            k_set_mode,   { UI_INSERT }, UI_NORMAL },
	/* FIXME: only if the previous succeeds, i.e. not c^[ */

	{ 'J',            k_join,       KEY_ARG_NONE,  UI_NORMAL },

	{ 'r',            k_replace,    { 0 },         UI_NORMAL },
	{ 'R', /* TODO */ k_replace,    { 1 },         UI_NORMAL },

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
	{ "wq",  c_x     },

	{ "vs",  c_vs    },
	{ "sp",  c_sp    },

	{ "!",   c_run   },

	{ NULL, NULL },
};

#endif

