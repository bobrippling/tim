#include <stdio.h>
#include <stdlib.h>

#include "pos.h"
#include "region.h"

#include "list.h"
#include "buffer.h"

#include "motion.h"

#include "str.h"

#include "surround.h"

static bool surround_via_motions(
		motion const *m1, unsigned repeat1,
		motion const *m2, unsigned repeat2,
		buffer_t *buf, region_t *surround)
{
	return motion_apply_buf_dry(m1, repeat1, buf, &surround->begin)
		&& motion_apply_buf_dry(m2, repeat2, buf, &surround->end);
}

bool surround_paren(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	/* essentially: [{v% */

	if(!paren_left(arg))
		arg = paren_opposite(arg);

	motion find_paren = {
		.func = m_paren,
		.arg.i = arg,
		.how = M_LINEWISE
	};
	motion match_paren = {
		.func = m_paren,
		.arg.i = '%',
		.how = M_LINEWISE
	};

	surround->type = REGION_CHAR;

	return surround_via_motions(
			&find_paren, repeat,
			&match_paren, 0,
			buf, surround);
}

bool surround_quote(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	return false;
}

bool surround_block(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	return false;
}

bool surround_para(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	return false;
}

bool surround_word(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	return false;
}
