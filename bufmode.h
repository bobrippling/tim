#ifndef BUFMODE_H
#define BUFMODE_H

enum buf_mode
{
	UI_NORMAL = 1 << 0,

	UI_INSERT = 1 << 1,
	UI_INSERT_COL = 1 << 2,

	UI_VISUAL_CHAR = 1 << 3,
	UI_VISUAL_COL = 1 << 4,
	UI_VISUAL_LN = 1 << 5,

#define UI_VISUAL_ANY (\
		UI_VISUAL_CHAR | \
		UI_VISUAL_COL  | \
		UI_VISUAL_LN)

#define UI_INSERT_ANY (UI_INSERT | UI_INSERT_COL)
};

#endif
