CFLAGS_WARN = -Wmissing-prototypes -Wno-unused-parameter -Wno-char-subscripts \
              -Wno-missing-field-initializers -Wno-format-zero-length

CFLAGS_DEF = -D_XOPEN_SOURCE

CFLAGS = -Wall -Wextra -pedantic -g -std=c11 -fms-extensions -fno-common \
         ${CFLAGS_WARN} ${CFLAGS_DEF}

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o \
	list.o buffers.o motion.o external.o str.o prompt.o io.o \
	yank.o pos.o region.o

tim: ${OBJ}
	cc -o $@ ${OBJ} -lncurses

check: tim
	cd test && ./run.pl

clean:
	rm -f ${OBJ} tim

deps:
	cc -MM ${OBJ:.o=.c} > Makefile.dep

include Makefile.dep
