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

	if(in_around == SURROUND_IN_CHAR){
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

bool surround_quote(
		const char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	list_t *line = buffer_current_line(buf, false);
	if(!line)
		return false;

	if((unsigned)buf->ui_pos->x >= line->len_line)
		return false;

	int i;
	for(i = buf->ui_pos->x; i >= 0; i--)
		if(line->line[i] == arg)
			break;
	if(i < 0)
		return false;

	const int start = i;
	for(i++; (unsigned)i < line->len_line; i++){
		if(line->line[i] == '\\')
			i++;
		else if(line->line[i] == arg)
			break;
	}
	if((unsigned)i >= line->len_line){
		/* >=, since an escape may have caused us to go past */
		return false;
	}

	*surround = (region_t){
		.type = REGION_CHAR,
		.begin = { .y = buf->ui_pos->y, .x = start },
		.end   = { .y = buf->ui_pos->y, .x = i },
	};

	return true;
}

bool surround_block(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	return surround_paren('(', repeat, buf, surround);
}

bool surround_para(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	list_t *line = buffer_current_line(buf, false);
	if(!line)
		return false;

	/* essentially: {jV} */

	motion up_para = {
		.func = m_aggregate,
		.arg.aggregate = {
			&(motion){
				.func = m_para,
				.arg.dir = -1,
				.how = M_LINEWISE
			},
			&(motion){
				.func = m_move,
				.arg.pos = { .y = +1 },
				.how = M_LINEWISE
			}
		}
	};
	motion down_para = {
		.func = m_para,
		.arg.dir = +1,
		.how = M_LINEWISE
	};

	surround->type = REGION_LINE;

	return surround_via_motions(
			&up_para, 0,
			&down_para, repeat,
			buf, surround);
}

bool surround_word(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	return false;
}
