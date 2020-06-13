#ifndef XV6_CMD_MODE_H
#define XV6_CMD_MODE_H

#include "vi.h"

char compare(char* a, const char* b);
line* newlines(uchar *chs, uint n);
int readtext(char *path, struct text* txx);
int writetext (char *path, line *head);

#endif //XV6_CMD_MODE_H
