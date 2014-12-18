CFLAGS_WARN = -Wmissing-prototypes -Wno-unused-parameter -Wno-char-subscripts \
              -Wno-missing-field-initializers -Wno-format-zero-length

CFLAGS_DEF = -D_XOPEN_SOURCE

CFLAGS = -Wall -Wextra -g -std=c11 -fms-extensions -fno-common \
         ${CFLAGS_WARN} ${CFLAGS_DEF}

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o \
	list.o motion.o external.o str.o prompt.o io.o \
	yank.o pos.o region.o retain.o range.o parse_cmd.o word.o \
	buffers.o window.o windows.o ctags.o

.PHONY: deps clean check checkmem

SRC = ${OBJ:.o=.c}

tim: ${OBJ}
	${CC} -o $@ ${OBJ} -lncurses

check: tim
	cd test && ./run.pl

checkmem: tim
	cd test && ./run.pl -v

clean:
	rm -f ${OBJ} tim

deps:
	${CC} -MM ${SRC} > Makefile.dep

tags:
	ctags ${SRC}

include Makefile.dep
.PHONY: check checkmem clean deps tags
