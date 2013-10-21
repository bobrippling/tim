#ifndef MOTION_H
#define MOTION_H

typedef union motion_arg motion_arg;

typedef int motion_func(
		motion_arg const *,
		unsigned repeat,
		buffer_t *,
		point_t *current, /* both changeable */
		point_t *to);

union motion_arg
{
	int i;
	enum find_type
	{
		F_TIL = 1 << 0, /* 'f' or 't' */
		F_REV = 1 << 1, /* 'F' or 'T' */
	} find_type;
	point_t pos;
	enum case_tog case_type;
};

motion_func m_eof, m_eol, m_eos, m_goto, m_mos, m_move, m_sof, m_sol, m_sos;
motion_func m_find, m_findnext;
motion_func m_word;
motion_func m_para;
motion_func m_search;
motion_func m_visual;

typedef struct motion
{
	motion_func *func;
	motion_arg arg;
	enum motion_wise
	{
		M_NONE      = 0,
		M_LINEWISE  = 1 << 0,
		M_EXCLUSIVE = 1 << 1,
	} how;
} motion;

#define MOTION_REPEAT() { NULL, 0U }
#define DEFAULT_REPEAT(r) (r ? r : 1)

int motion_apply_buf(const motion *, unsigned rep, buffer_t *);

#endif
