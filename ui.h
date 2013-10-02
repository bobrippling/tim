#ifndef UI_H
#define UI_H

#ifdef __GNUC__
#  define tim_printf(a, b) __attribute__((format(printf, a, b)))
#else
#  define tim_printf(a, b)
#endif

enum ui_mode
{
	UI_NORMAL,
	UI_INSERT,
};

void ui_set_mode(enum ui_mode);
void ui_init(void);
void ui_main(void);
void ui_term(void);

void ui_redraw(void);
void ui_cur_changed(void);
void ui_clear(void); /* full clear, for ^L */

void ui_status(const char *fmt, ...) tim_printf(1, 2);

extern int ui_running;

extern enum ui_mode ui_mode;

#endif
