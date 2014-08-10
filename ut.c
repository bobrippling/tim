#include <stddef.h>
#include <assert.h>

#include "str.h"

int main(int argc, char **argv)
{
	assert(argc == 1);

	assert(str_mixedcase("abcD"));
	assert(str_mixedcase("24j 41j #12#j14 14k j1%j@kj%3j~@%jA"));

	assert(!str_mixedcase("abcd"));
	assert(!str_mixedcase(""));
	assert(!str_mixedcase("24j 41j #12#j14 14k j1%j@kj%3j~@%j"));

	return 0;
}
