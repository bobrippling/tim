#ifndef INDENT_H
#define INDENT_H

void indent_adjust(list_t *l, unsigned xlim, int *const indent);
int indent_count(list_t *l, bool include_brackets);

#endif
