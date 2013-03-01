#ifndef MOTION_H
#define MOTION_H

typedef union motion_arg motion_arg;

typedef void motion_func(
		motion_arg const *,
		buffer_t *,
		point_t *);

union motion_arg
{
	int i;
	point_t pos;
};

motion_func m_eof, m_eol, m_eos, m_goto, m_mos, m_move, m_sof, m_sol, m_sos;

typedef struct motion
{
	motion_func *func;
	motion_arg arg;
} motion;

void motion_apply_buf(const motion *, buffer_t *buf);

#endif
