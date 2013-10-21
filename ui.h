#ifndef UI_H
#define UI_H

#ifdef __GNUC__
#  define tim_printf(a, b) __attribute__((format(printf, a, b)))
#else
#  define tim_printf(a, b)
#endif

void ui_init(void);
void ui_main(void);
void ui_term(void);

void ui_redraw(void);
void ui_cur_changed(void);
void ui_clear(void); /* full clear, for ^L */

void ui_status(const char *fmt, ...) tim_printf(1, 2);

#ifdef BUFFER_H
void ui_set_bufmode(enum buf_mode m);
#endif

extern int ui_running;

#endif
