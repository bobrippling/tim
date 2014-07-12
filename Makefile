CFLAGS_WARN = -Wmissing-prototypes -Wno-unused-parameter -Wno-char-subscripts \
              -Wno-missing-field-initializers -Wno-format-zero-length

CFLAGS_DEF = -D_XOPEN_SOURCE

CFLAGS = -Wall -Wextra -g -std=c11 -fms-extensions -fno-common \
         ${CFLAGS_WARN} ${CFLAGS_DEF}

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o \
	list.o buffers.o motion.o external.o str.o prompt.o io.o \
	yank.o pos.o region.o retain.o range.o parse_cmd.o

.PHONY: deps clean check checkmem
.PHONY: deps clean all check checkmem

all: tim gq

tim: ${OBJ}
	cc -o $@ ${OBJ} -lncurses

check: tim
	cd test && ./run.pl

checkmem: tim
	cd test && ./run.pl -v

clean:
	rm -f ${OBJ} tim

deps:
	cc -MM ${OBJ:.o=.c} > Makefile.dep

include Makefile.dep
.PHONY: check checkmem clean deps
