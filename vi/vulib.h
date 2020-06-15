#ifndef XV6_CMD_MODE_H
#define XV6_CMD_MODE_H

#include "vi.h"

line* newlines(uchar *chs, uint n);
int readtext(char *path, struct text* txx);
int writetext (char *path, line *head);
char* getfilename(char *path);
#endif //XV6_CMD_MODE_H
