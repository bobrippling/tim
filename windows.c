#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"

#include "window.h"
#include "windows.h"

#include "tab.h"
#include "tabs.h"

window *windows_cur()
{
	tab *t = tabs_cur();

	return t ? t->win : NULL;
}

void windows_set_cur(window *new)
{
	tab_set_focus(tabs_cur(), new);
}
