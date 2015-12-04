#ifndef EXTERNAL_H
#define EXTERNAL_H

void shellout_wait_ret(void);
int shellout(const char *cmd);

bool shellout_write(const char *cmd, list_t *seeked, int nlines);
bool shellout_read_write(const char *cmd, list_t *list, int nlines, list_t **const out);

#endif
