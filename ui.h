#ifndef UI_H
#define UI_H

enum ui_mode
{
	UI_NORMAL = 1 << 0,
	UI_INSERT = 1 << 1
};

void ui_init(void);
void ui_main(void);
void ui_term(void);

void ui_redraw(void);
void ui_cur_changed(void);

void ui_status(const char *fmt, ...);

extern int ui_running;
extern int ui_y, ui_x;

extern enum ui_mode ui_mode;

#endif
