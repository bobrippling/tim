#ifndef CONFIG_H
#define CONFIG_H

#define KEY_ARG_NONE        { 0 }
#define MOTION_ARG_NONE     { 0 }

const motionkey_t motion_keys[] = {
	{ "w",  { m_word, { .word_type = WORD_NONE                }, M_EXCLUSIVE } },
	{ "b",  { m_word, { .word_type = WORD_BACKWARD            }, M_EXCLUSIVE } },
	{ "e",  { m_word, { .word_type = WORD_END                 }, M_EXCLUSIVE } },
	{ "ge", { m_word, { .word_type = WORD_END | WORD_BACKWARD }, M_EXCLUSIVE } },

	{ "W",  { m_word, { .word_type = WORD_SPACE }, M_EXCLUSIVE } },
	{ "B",  { m_word, { .word_type = WORD_BACKWARD | WORD_SPACE }, M_EXCLUSIVE } },
	{ "E",  { m_word, { .word_type = WORD_END | WORD_SPACE }, M_EXCLUSIVE } },
	{ "gE", { m_word, { .word_type = WORD_END | WORD_BACKWARD | WORD_SPACE }, M_EXCLUSIVE } },

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

	{ "gg",           { m_sof,  MOTION_ARG_NONE, M_LINEWISE } }, // FIXME: goto
	{ "G",            { m_eof,  MOTION_ARG_NONE, M_LINEWISE } },

	{ "H",            { m_sos,  MOTION_ARG_NONE, M_LINEWISE } },
	{ "M",            { m_mos,  MOTION_ARG_NONE, M_LINEWISE } },
	{ "L",            { m_eos,  MOTION_ARG_NONE, M_LINEWISE } },

	{ "/",            { m_search, { +1 }, M_EXCLUSIVE } },
	{ "?",            { m_search, { -1 }, M_EXCLUSIVE } },
	{ "n",            { m_searchnext, { +1 }, M_EXCLUSIVE } },
	{ "N",            { m_searchnext, { -1 }, M_EXCLUSIVE } },
};
const size_t motion_keys_cnt = sizeof motion_keys / sizeof *motion_keys;

#define K_STR(k) (char[]){ k, 0 }
const nkey_t nkeys[] = {
	/* order is important */
	/* char, func, arg, mode */
	{ K_STR(K_ESC), k_escape, KEY_ARG_NONE, UI_INSERT_ANY | UI_VISUAL_ANY },

	{ "i",            k_set_mode,   { UI_INSERT }, UI_NORMAL },

	{ "V",            k_set_mode,   { UI_VISUAL_LN }, UI_NORMAL | UI_VISUAL_ANY },
	{ "v",            k_set_mode,   { UI_VISUAL_CHAR }, UI_NORMAL | UI_VISUAL_ANY },
	{ K_STR(CTRL_AND('v')), k_set_mode, { UI_VISUAL_COL }, UI_NORMAL | UI_VISUAL_ANY },

	{ "o",            k_open,       {  1 },                   UI_NORMAL },
	{ "O",            k_open,       { -1 },                   UI_NORMAL },
	{ "o",            k_vtoggle,    KEY_ARG_NONE, UI_VISUAL_ANY },

	{ "d",            k_del,        KEY_ARG_NONE,  UI_NORMAL | UI_VISUAL_ANY },
	{ "c",            k_change,     KEY_ARG_NONE,  UI_NORMAL | UI_VISUAL_ANY },

	{ "J",            k_join,       KEY_ARG_NONE,  UI_NORMAL | UI_VISUAL_ANY },

	{ ">",            k_indent,     { +1 },  UI_NORMAL | UI_VISUAL_ANY },
	{ "<",            k_indent,     { -1 },  UI_NORMAL | UI_VISUAL_ANY },

	{ "r",            k_replace,    { 0 },         UI_NORMAL | UI_VISUAL_ANY },
	{ "R", /* TODO */ k_replace,    { 1 },         UI_NORMAL | UI_VISUAL_ANY },

	{ ":",            k_cmd,        KEY_ARG_NONE,            UI_NORMAL | UI_VISUAL_ANY }, /* k_set_mode instead? */

	{ "!",            k_filter,     { .filter = { FILTER_CMD, NULL }}, UI_NORMAL | UI_VISUAL_ANY },
	{ "g!",           k_filter,     { .filter.type = FILTER_SELF }, UI_NORMAL | UI_VISUAL_ANY },
	{ "gq",           k_filter,     { .filter = { FILTER_CMD, "fmt" }}, UI_NORMAL | UI_VISUAL_ANY },

	{ "~",            k_case, { CASE_TOGGLE }, UI_NORMAL | UI_VISUAL_ANY },
	{ "gU",           k_case, { CASE_UPPER  }, UI_NORMAL | UI_VISUAL_ANY },
	{ "gu",           k_case, { CASE_LOWER  }, UI_NORMAL | UI_VISUAL_ANY },

	{ K_STR(CTRL_AND('l')),  k_redraw,     KEY_ARG_NONE,            UI_NORMAL | UI_VISUAL_ANY },

	{ K_STR(CTRL_AND('e')),  k_scroll,     { .pos = { 0,  2 } },     UI_NORMAL | UI_VISUAL_ANY },
	{ K_STR(CTRL_AND('y')),  k_scroll,     { .pos = { 0, -2 } },     UI_NORMAL | UI_VISUAL_ANY },

	{ "zz",  k_scroll,     { .pos = { 1,  0 } },     UI_NORMAL | UI_VISUAL_ANY },
	{ "zt",  k_scroll,     { .pos = { 1, -1 } },     UI_NORMAL | UI_VISUAL_ANY },
	{ "zb",  k_scroll,     { .pos = { 1, +1 } },     UI_NORMAL | UI_VISUAL_ANY },

	{ K_STR(CTRL_AND('w')),  k_winsel,     KEY_ARG_NONE,             UI_NORMAL | UI_VISUAL_ANY },

	{ K_STR(CTRL_AND('g')),  k_show,       KEY_ARG_NONE,             UI_NORMAL | UI_VISUAL_ANY },
};
const size_t nkeys_cnt = sizeof nkeys / sizeof *nkeys;

const keymap_t maps[] = {
	{ IO_MAP, 'I', "^i" },
	{ IO_MAP, 'a', "li" },
	{ IO_MAP, 'A', "$li" }, /* remap not allowed, hence "$a" wouldn't work */

	{ IO_MAP, 'C', "c$" },
	{ IO_MAP, 'D', "d$" },
	{ IO_MAPV, 'C', "c" },
	{ IO_MAPV, 'D', "d" },

	/* TODO: delete command needs to handle count */
	{ IO_MAP, 'x', "dl" },
	{ IO_MAP, 'X', "dh" },
	{ IO_MAPV, 'x', "d" },
	{ IO_MAPV, 'X', "d" },

	/* TODO: change command needs to handle count */
	{ IO_MAP, 's', "cl" },
	{ IO_MAP, 'S', "0c$" },
	{ IO_MAPV, 's', "c" },

	/* insert mode keys - ^U, ^K and ^W */
	{ IO_MAPI, CTRL_AND('U'), (char[]){ K_ESC, 'l', 'd', '0', 'i', 0 } },
	{ IO_MAPI, CTRL_AND('K'), (char[]){ K_ESC, 'l', 'd', '$', 'i', 0 } },
	{ IO_MAPI, CTRL_AND('W'), (char[]){ K_ESC, 'l', 'd', 'b', 'i', 0 } },

	{ 0 }
};

const cmd_t cmds[] = {
	{ "q",   c_q     },
	{ "cq",  c_cq    },
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

