#ifndef MOTION_H
#define MOTION_H

typedef union motion_arg motion_arg;

typedef int motion_func(
		motion_arg const *,
		unsigned repeat,
		window *,
		/*point_t *current, * both changeable */
		point_t *to);

enum motion_wise
{
	M_NONE      = 0,
	M_LINEWISE  = 1 << 0,
	M_EXCLUSIVE = 1 << 1,
	M_COLUMN    = 1 << 2,
};

enum word_wise
{
	WORD_NONE = 0,
	WORD_BACKWARD = 1 << 0, /* b,B,ge,gE not w,W,e,E */
	WORD_END = 1 << 1, /* e,E,ge,gE not w,W,b,B */
	WORD_SPACE = 1 << 2, /* W, E, B, gE */
};

union motion_arg
{
	int i;
	enum word_wise word_type;
	enum find_type
	{
		F_TIL = 1 << 0, /* 'f' or 't' */
		F_REV = 1 << 1, /* 'F' or 'T' */
	} find_type;
	point_t pos;
	enum case_tog case_type;
	enum motion_wise *phow;
	struct
	{
		signed char dir;
		char scan_ch;
	};
};

motion_func m_eof, m_eol, m_eos, m_goto, m_mos, m_move, m_sof, m_sol, m_sos;
motion_func m_find, m_findnext;
motion_func m_word;
motion_func m_para, m_paren, m_func;
motion_func m_search, m_searchnext;
motion_func m_visual;

typedef struct motion
{
	motion_func *func;
	motion_arg arg;
	enum motion_wise how;
} motion;

#define MOTION_REPEAT() { NULL, 0U }
#define DEFAULT_REPEAT(r) (r ? r : 1)

int motion_apply_win(
		const motion *, unsigned rep,
		window *);

int motion_apply_win_dry(
		const motion *, unsigned rep,
		window *, point_t *to);

bool motion_to_region(
		const motion *m, unsigned repeat, bool always_linewise,
		window *, region_t *out);

#endif
