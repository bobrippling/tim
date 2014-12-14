/* wrap text to 80 columns
 *
 * TODO:
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

enum
{
	WIDTH = 80
};

struct charbuf
{
	char *p;
	size_t len;

	char *anchor;
	size_t alloc;
};


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

static void charbuf_out_n(struct charbuf *cb, FILE *stream, size_t n)
{
	fwrite(cb->p, 1, n, stream);
}

static void charbuf_out(struct charbuf *cb, FILE *stream)
{
	charbuf_out_n(cb, stream, cb->len);
}

static void handle_line(struct charbuf *const in)
{
	while(in->len > WIDTH - 1){
		charbuf_out_n(in, stdout, WIDTH - 1);
		fputc('\n', stdout);
		in->len -= WIDTH - 1;
		in->p += WIDTH - 1;

	}

	charbuf_out(in, stdout);
}

int main(int argc, const char *argv[])
{
	argv0 = *argv;

	if(argc > 1)
		usage();

	struct charbuf buf = { 0 };

	for(;;){
		ssize_t n = getline(&buf.anchor, &buf.alloc, stdin);
		if(n < 0){
			if(ferror(stdin))
				die("read():");
			break;
		}

		buf.p = buf.anchor;
		buf.len = n;

		handle_line(&buf);
	}

	return 0;
}
