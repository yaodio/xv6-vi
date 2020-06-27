#ifndef VI_XV6_EDITMODE_H
#define VI_XV6_EDITMODE_H

int insertc(line *l, int i, uchar c);
int deletec(line *l, int i);
void breakline(line *l, int i);
int editmode(void);

#endif // VI_XV6_EDITMODE_H