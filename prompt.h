#ifndef PROMPT_H
#define PROMPT_H

enum prompt_expansion
{
	PROMPT_NONE,
	PROMPT_FILENAME,
	PROMPT_EXEC,
};

char *prompt(char, enum prompt_expansion);

#endif
