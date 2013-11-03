CFLAGS = -Wall -Wextra -pedantic -g -std=c99 \
				 -fms-extensions -fno-common         \
				 -Wmissing-prototypes -Wno-unused-parameter \
				 -Wno-char-subscripts -Wno-missing-field-initializers \
				 -D_POSIX_SOURCE=1

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o \
	list.o buffers.o motion.o external.o str.o prompt.o io.o \
	pos.o region.o

tim: ${OBJ}
	cc -o $@ ${OBJ} -lncurses

check: tim
	cd test && ./run.pl

clean:
	rm -f ${OBJ} tim

deps:
	cc -MM ${OBJ:.o=.c} > Makefile.dep

include Makefile.dep
