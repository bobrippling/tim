/* wrap text to 80 columns
 *
 * also if text begins with a - and following lines don't,
 * indent like so:
 *
 * - a
 *   - abc hello
 *   - abcdef long long ...
 *     lots of words
 * - xyz
 *
 * Also C comment gq'ing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <stdnoreturn.h>

#include <unistd.h>

static const char *argv0;

static noreturn void usage(void)
{
	fprintf(stderr, "Usage: %s\n  stdin -> stdout\n", argv0);
	exit(2);
}

static noreturn void die(const char *emsg)
{
	fprintf(stderr, "%s", emsg);
	if(*emsg && emsg[strlen(emsg) - 1] == ':'){
		fprintf(stderr, " %s", strerror(errno));
	}
	fprintf(stderr, "\n");
	exit(1);
}

static void remove_nl(char buf[], size_t *n)
{
	char *nl = buf;
	size_t total = 0;

	for(;;){
		nl = memchr(nl, '\n', *n);
		if(!nl)
			break;

		memmove(nl, nl + 1, *n - (nl - buf));
		total++;
	}

	*n -= total;
}

int main(int argc, const char *argv[])
{
	argv0 = *argv;

	if(argc > 1)
		usage();

	char *accum = NULL, buf[256];
	size_t nacc = 0;

	for(;;){
		size_t n = fread(buf, 1, sizeof buf, stdin);
		if(n == 0){
			if(ferror(stdin))
				die("read():");
			break;
		}

		remove_nl(buf, &n);

		char *tmp = realloc(accum, nacc += n);
		if(!tmp)
			die("malloc():");
		accum = tmp;
		memcpy(accum + nacc - n, buf, n);
	}

	fprintf(stderr, "acc = '%s'\n", accum);
}
