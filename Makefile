CFLAGS_WARN = -Wmissing-prototypes -Wno-unused-parameter -Wno-char-subscripts \
              -Wno-missing-field-initializers -Wno-format-zero-length

CFLAGS_DEF = -D_XOPEN_SOURCE

CFLAGS = -Wall -Wextra -g -std=c11 -fms-extensions -fno-common \
         ${CFLAGS_WARN} ${CFLAGS_DEF}

LDFLAGS = -lncurses -g
LDFLAGS_STATIC = -static ${LDFLAGS} -ltinfo -lgpm

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o \
	list.o motion.o external.o str.o prompt.o io.o \
	yank.o pos.o region.o retain.o range.o parse_cmd.o word.o \
	buffers.o window.o windows.o tab.o tabs.o ctags.o

SRC = ${OBJ:.o=.c}

Q = @

tim: ${OBJ}
	@echo link $@
	$Q${CC} -o $@ ${OBJ} ${LDFLAGS}

%.o: %.c
	@echo compile $<
	$Q${CC} -c -o $@ $< ${CFLAGS}

tim.static: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS_STATIC}

check: tim
	cd test && ./run.pl

checkmem: tim
	cd test && ./run.pl -v

clean:
	@echo clean
	$Qrm -f ${OBJ} tim

tags:
	@echo ctags
	$Q-ctags ${SRC}

.%.d: %.c
	@echo depend $<
	$Q${CC} -MM -o $@ $< ${CPPFLAGS}

-include ${OBJ:%.o=.%.d}

.PHONY: check checkmem clean tags
