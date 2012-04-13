#ifndef MOTION_H
#define MOTION_H

typedef union Motion Motion;

typedef void motion_func(Motion *);

union Motion
{
	int i;
	struct
	{
		int x, y;
	} pos;
};

motion_func m_eof, m_eol, m_eos, m_goto, m_mos, m_move, m_sof, m_sol, m_sos;

#endif
