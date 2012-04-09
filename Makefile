CFLAGS = -Wall -Wextra -pedantic -g

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o buffer.o list.o buffers.o

tim: ${OBJ}
	cc -o $@ ${OBJ} -lncurses

clean:
	rm -f *.o tim

buffer.o: buffer.c list.h buffer.h mem.h
buffers.o: buffers.c list.h buffer.h buffers.h mem.h ui.h
cmds.o: cmds.c cmds.h ui.h list.h buffer.h buffers.h
keys.o: keys.c ui.h keys.h ncurses.h mem.h cmds.h list.h buffer.h \
 buffers.h config.h
list.o: list.c list.h mem.h
main.o: main.c ui.h list.h buffer.h buffers.h
mem.o: mem.c mem.h
ncurses.o: ncurses.c ncurses.h
ui.o: ui.c ui.h ncurses.h keys.h list.h buffer.h buffers.h
