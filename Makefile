CFLAGS = -Wall -Wextra -pedantic -g -std=c99 \
				 -fms-extensions -Wno-microsoft      \
				 -Wmissing-prototypes -Wno-unused-parameter

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o \
	list.o buffers.o motion.o external.o

tim: ${OBJ}
	cc -o $@ ${OBJ} -lncurses

clean:
	rm -f *.o tim

buffer.o: buffer.c pos.h list.h buffer.h mem.h
buffers.o: buffers.c pos.h list.h buffer.h buffers.h mem.h ui.h
cmds.o: cmds.c cmds.h ui.h pos.h list.h buffer.h buffers.h external.h \
  mem.h
external.o: external.c mem.h ui.h external.h
keys.o: keys.c pos.h list.h buffer.h ui.h motion.h keys.h ncurses.h mem.h \
  cmds.h buffers.h config.h
list.o: list.c pos.h list.h mem.h
main.o: main.c ui.h pos.h list.h buffer.h buffers.h
mem.o: mem.c mem.h
motion.o: motion.c pos.h list.h buffer.h motion.h ui.h
ncurses.o: ncurses.c ncurses.h
ui.o: ui.c pos.h ui.h ncurses.h list.h buffer.h motion.h keys.h buffers.h
