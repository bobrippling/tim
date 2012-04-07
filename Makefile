CFLAGS = -Wall -Wextra -pedantic

OBJ = main.o ncurses.o ui.o mem.o keys.o cmds.o

tim: ${OBJ}
	cc -o $@ ${OBJ} -lncurses

clean:
	rm -f *.o tim

cmds.o: cmds.c cmds.h ui.h
keys.o: keys.c keys.h ncurses.h mem.h config.h
main.o: main.c ui.h
mem.o: mem.c mem.h
ncurses.o: ncurses.c ncurses.h
ui.o: ui.c ui.h ncurses.h keys.h
