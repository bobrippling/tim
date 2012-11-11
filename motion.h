#ifndef MOTION_H
#define MOTION_H

typedef union motion_arg motion_arg;

typedef void motion_func(motion_arg *, buffer_t *, point_t const *, point_t *);

union motion_arg
{
	int i;
	point_t pos;
};

motion_func m_eof, m_eol, m_eos, m_goto, m_mos, m_move, m_sof, m_sol, m_sos;

void motion_apply(    motionkey_t *m, buffer_t *buf, point_t *to);
void motion_apply_buf(motionkey_t *m, buffer_t *buf);

#endif
