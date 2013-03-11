#ifndef MOTION_H
#define MOTION_H

typedef union motion_arg motion_arg;

typedef void motion_func(
		motion_arg const *,
		unsigned repeat,
		const buffer_t *,
		point_t *);

union motion_arg
{
	int i;
	point_t pos;
};

motion_func m_eof, m_eol, m_eos, m_goto, m_mos, m_move, m_sof, m_sol, m_sos;
motion_func m_find;

typedef struct motion
{
	motion_func *func;
	motion_arg arg;
} motion;

typedef struct motion_repeat
{
	const motion *motion;
	unsigned repeat;
} motion_repeat;

#define MOTION_REPEAT() { NULL, 0U }
#define DEFAULT_REPEAT(r) (r ? r : 1)

void motion_apply_buf_dry(const motion_repeat *, const buffer_t *, point_t *out);
void motion_apply_buf(const motion_repeat *, buffer_t *);

#endif
