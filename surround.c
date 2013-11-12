#include <stdio.h>
#include <stdlib.h>

#include "pos.h"
#include "region.h"

#include "list.h"
#include "buffer.h"

#include "surround.h"

bool surround_paren(
		char arg, unsigned repeat,
		buffer_t *buf, region_t *surround)
{
	return false;
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
