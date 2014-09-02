#include <stdio.h>
#include <stdlib.h>

#include "pos.h"
#include "region.h"

#include "list.h"
#include "buffer.h"

#include "motion.h"

#include "str.h"

#include "surround.h"

bool surround_apply(const surround_key_t *k,
		char in_around, char type, unsigned repeat,
		buffer_t *buf, region_t *r)
{
	if(!k->func(type, repeat, buf, r))
		return false;

	if(in_around == 'i'){
		int xdiff = r->end.x - r->begin.x;

		if(r->end.y - r->begin.y == 0){
			if(xdiff == 0)
				return false;
		}else{
			xdiff = 1; /* anything > 0 */
		}

		list_advance_x(
				list_seek(buf->head, r->begin.y, false),
				+1, &r->begin.y, &r->begin.x);

		if(xdiff > 0){
			list_advance_x(
					list_seek(buf->head, r->end.y, false),
					-1, &r->end.y, &r->end.x);
		}
	}
	return true;
}

static bool surround_via_motions(
		motion const *m1, unsigned repeat1,
		motion const *m2, unsigned repeat2,
		buffer_t *buf, region_t *surround)
{
	return
		motion_apply_buf_dry(m1, repeat1, buf,
				buf->ui_pos, &surround->begin)
		&&
		motion_apply_buf_dry(m2, repeat2, buf,
				&surround->begin, &surround->end);
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

#warning todo
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