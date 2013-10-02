CFLAGS = -Wall -Wextra -pedantic -g -std=c99 \
				 -fms-extensions                     \
				 -Wmissing-prototypes -Wno-unused-parameter \
				 -Wno-char-subscripts -Wno-missing-field-initializers

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o \
	list.o buffers.o motion.o external.o str.o prompt.o io.o

tim: ${OBJ}
	cc -o $@ ${OBJ} -lncurses

clean:
	rm -f ${OBJ} tim

deps:
	cc -MM ${OBJ:.o=.c} > Makefile.dep

include Makefile.dep
