#ifndef VI_XV6_vulib_H
#define VI_XV6_vulib_H

uchar readc(void);
char* getfilename(char *path);
void* calloc(uint size_a, uint size_t);
int int2char(char* res, int xx);
int startswidth(uchar *chs, uchar *prefix, int n);

#endif //VI_XV6_vulib_H
