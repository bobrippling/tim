#ifndef TABS_H
#define TABS_H

tab *tabs_cur(void);
void tabs_set_cur(tab *);

enum init_args
{
	INIT_NONE = 0,
	INIT_VALL,
	INIT_HALL
};

void tabs_init(enum init_args, unsigned off);
void tabs_term(void);

#define tabs_iter(tab) \
	for(tab *tab = tabs_cur(), *const begin = tab; tab->next != begin; tab = tab->next)

#endif