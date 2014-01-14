#ifndef KEYMACROS_H
#define KEYMACROS_H

#define K_ESC '\033'

#define CTRL_AND(c)  ((c) & 037)

#define case_BACKSPACE     \
			case CTRL_AND('?'):  \
			case CTRL_AND('H'):  \
			case 263:            \
			case 127

#endif
