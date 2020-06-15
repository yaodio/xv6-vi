#ifndef XV6_CMD_MODE_H
#define XV6_CMD_MODE_H

#include "vi.h"

line* newlines(uchar *chs, uint n);
void setline(line *l, uchar *chs, int n, uchar color);
void cleanline(line *l);

int readtext(char *path, struct text* txx);
int writetext (char *path, line *head);
char* getfilename(char *path);
void* calloc(uint size_a, uint size_t);

#endif //XV6_CMD_MODE_H
