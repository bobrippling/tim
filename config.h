#ifndef CONFIG_H
#define CONFIG_H

#define KEY_ARG_NONE        { 0 }
#define MOTION_ARG_NONE     { 0 }

const motionkey_t motion_keys[] = {
	{ "w",            { m_word, {  1 }, M_EXCLUSIVE } },
	{ "b",            { m_word, { -1 }, M_EXCLUSIVE } },

	{ "j",            { m_move, { .pos = {  0,  1 } }, M_LINEWISE } },
	{ "k",            { m_move, { .pos = {  0, -1 } }, M_LINEWISE } },
	{ "h",            { m_move, { .pos = { -1,  0 } }, M_EXCLUSIVE } },
	{ "l",            { m_move, { .pos = {  1,  0 } }, M_EXCLUSIVE } },

	{ "{",            { m_para,     { -1 }, M_LINEWISE } },
	{ "}",            { m_para,     { +1 }, M_LINEWISE } },
	{ "%",            { m_paren,    { '%' }, M_NONE } },

	{ "[[", { m_func, { -1 }, M_LINEWISE } },
	{ "]]", { m_func, { +1 }, M_LINEWISE } },

	{ "[{", { m_paren, { '{' }, M_LINEWISE } },
	{ "[(", { m_paren, { '(' }, M_LINEWISE } },
	{ "]}", { m_paren, { '}' }, M_LINEWISE } },
	{ "])", { m_paren, { ')' }, M_LINEWISE } },

	{ "f",            { m_find,     { .find_type = 0             }, M_NONE } },
	{ "F",            { m_find,     { .find_type = F_REV         }, M_NONE } },
	{ "t",            { m_find,     { .find_type = F_TIL         }, M_NONE } },
	{ "T",            { m_find,     { .find_type = F_REV | F_TIL }, M_NONE } },
	{ ";",            { m_findnext, { .find_type = 0             }, M_NONE } },
	{ ",",            { m_findnext, { .find_type = F_REV         }, M_NONE } },

	{ "0",            { m_goto, { .pos = { 0, -1 } }, M_EXCLUSIVE } },
	{ "^",            { m_sol,  MOTION_ARG_NONE,      M_EXCLUSIVE } },
	{ "$",            { m_eol,  MOTION_ARG_NONE,      M_NONE      } },

	{ "g",            { m_sof,  MOTION_ARG_NONE, M_LINEWISE } }, // FIXME: goto
	{ "G",            { m_eof,  MOTION_ARG_NONE, M_LINEWISE } },

	{ "H",            { m_sos,  MOTION_ARG_NONE, M_LINEWISE } },
	{ "M",            { m_mos,  MOTION_ARG_NONE, M_LINEWISE } },
	{ "L",            { m_eos,  MOTION_ARG_NONE, M_LINEWISE } },

	{ "/",            { m_search, { +1 }, M_EXCLUSIVE } },
	{ "?",            { m_search, { -1 }, M_EXCLUSIVE } },
	{ "n",            { m_searchnext, { +1 }, M_EXCLUSIVE } },
	{ "N",            { m_searchnext, { -1 }, M_EXCLUSIVE } },

	{ 0 }
};

const nkey_t nkeys[] = {
	/* order is important */
	/* char, func, arg, mode */
	{ K_ESC, k_motion, { .motion = { .m = { m_move, .arg.pos = { -1, 0 } }, .repeat = 1 } }, UI_INSERT_ANY },
	{ K_ESC,         k_set_mode,   { UI_NORMAL }, UI_INSERT_ANY | UI_VISUAL_ANY },

	{ 'i',            k_set_mode,   { UI_INSERT }, UI_NORMAL },

	{ 'V',            k_set_mode,   { UI_VISUAL_LN }, UI_NORMAL | UI_VISUAL_ANY },
	{ 'v',            k_set_mode,   { UI_VISUAL_CHAR }, UI_NORMAL | UI_VISUAL_ANY },
	{ CTRL_AND('v'),  k_set_mode,   { UI_VISUAL_COL }, UI_NORMAL | UI_VISUAL_ANY },

	{ 'o',            k_open,       {  1 },                   UI_NORMAL },
	{ 'O',            k_open,       { -1 },                   UI_NORMAL },
	{ 'o',            k_vtoggle,    KEY_ARG_NONE, UI_VISUAL_ANY },

	{ 'd',            k_del,        KEY_ARG_NONE,  UI_NORMAL | UI_VISUAL_ANY },
	{ 'c',            k_change,     KEY_ARG_NONE,  UI_NORMAL | UI_VISUAL_ANY },

	{ 'y',            k_yank, KEY_ARG_NONE, UI_NORMAL | UI_VISUAL_ANY },
	{ 'p',            k_put, { false }, UI_NORMAL },
	{ 'P',            k_put, { true }, UI_NORMAL },
	/* TODO: p-in-visual */

	{ 'J',            k_join,       KEY_ARG_NONE,  UI_NORMAL | UI_VISUAL_ANY },

	{ '>',            k_indent,     { +1 },  UI_NORMAL | UI_VISUAL_ANY },
	{ '<',            k_indent,     { -1 },  UI_NORMAL | UI_VISUAL_ANY },

	{ 'r',            k_replace,    { 0 },         UI_NORMAL | UI_VISUAL_ANY },
	{ 'R', /* TODO */ k_replace,    { 1 },         UI_NORMAL | UI_VISUAL_ANY },

	{ ':',            k_cmd,        KEY_ARG_NONE,            UI_NORMAL | UI_VISUAL_ANY }, /* k_set_mode instead? */

	{ '!',            k_filter,     { .s = NULL }, UI_NORMAL | UI_VISUAL_ANY },
	{ 'q',            k_filter,     { .s = "fmt" }, UI_NORMAL | UI_VISUAL_ANY },

	{ '~',            k_case, { CASE_TOGGLE }, UI_NORMAL | UI_VISUAL_ANY },
/*{ "gU", TODO      k_case, { CASE_UPPER  }, UI_NORMAL | UI_VISUAL_ANY },*/
/*{ "gu",           k_case, { CASE_LOWER  }, UI_NORMAL | UI_VISUAL_ANY },*/

	{ CTRL_AND('l'),  k_redraw,     KEY_ARG_NONE,            UI_NORMAL | UI_VISUAL_ANY },

	{ CTRL_AND('e'),  k_scroll,     { .pos = { 0,  2 } },     UI_NORMAL | UI_VISUAL_ANY },
	{ CTRL_AND('y'),  k_scroll,     { .pos = { 0, -2 } },     UI_NORMAL | UI_VISUAL_ANY },

	{ CTRL_AND('w'),  k_winsel,     KEY_ARG_NONE,             UI_NORMAL | UI_VISUAL_ANY },

	{ CTRL_AND('g'),  k_show,       KEY_ARG_NONE,             UI_NORMAL | UI_VISUAL_ANY },


	{ 0 }
};

const keymap_t maps[] = {
	{ 'I', "^i", NULL },
	{ 'a', "li", NULL },
	{ 'A', "$li", NULL }, /* remap not allowed, hence "$a" wouldn't work */
	{ 'C', "c$", "c" },
	{ 'D', "d$", "d" },

	{ 'x', "dl", "d" }, /* TODO: delete command needs to handle count */
	{ 'X', "dh", "d" },

	{ 'Y', "y$", "y" },

	{ 's', "cl", "c" }, /* TODO: change command needs to handle count */
	{ 'S', "0c$", NULL },

	{ 0 }
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

	{ NULL }
};

#endif

