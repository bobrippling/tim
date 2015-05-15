#include <stdio.h>
#include <ctype.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "indent.h"

static void adjust_brace_indent(char ch, int *const indent)
{
	switch(ch){
		case '{': (*indent)++; break;
		case '}': (*indent)--; break;
		case '(': (*indent) += 2; break;
		case ')': (*indent) -= 2; break;
	}
}

void indent_adjust(list_t *l, unsigned xlim, int *const indent)
{
	if((unsigned)xlim > l->len_line)
		xlim = l->len_line;

	for(unsigned i = 0; i < xlim; i++)
		adjust_brace_indent(l->line[i], indent);
}

int indent_count(list_t *l, bool include_brackets)
{
	int indent = 0;
	bool on_whitespace = true;

	/* count indent */
	for(unsigned i = 0; i < l->len_line; i++){
		if(on_whitespace){
			if(isspace(l->line[i])){
				indent++;
			}else{
				on_whitespace = false;
				if(!include_brackets)
					break;
			}
		}

		if(!on_whitespace)
			adjust_brace_indent(l->line[i], &indent);
	}

	return indent;
}
