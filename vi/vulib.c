#include "vi.h"

// 从标准输入（键盘）读入1个字符
uchar
readc(void)
{
  static uchar buf[1];

  while (1){
    if(read(0, buf, sizeof(buf[0])) > 0 && buf[0] != 0)
      return buf[0];
  }
}

char*
getfilename(char *path)
{
  int i, len;
  char* name;

  if((len = strlen(path)) == 0)
    return NULL;
  
  for(i = len - 1; i>=0 && path[i] != '/'; i--)
    ;
  
  name = (char*)malloc(len-i);
  memmove(name, path+i+1, len-i);
  return name;
}

void*
calloc(uint size_a, uint size_t)
{
  return malloc(size_a * size_t);
}

// 给定整数xx，转换成字符串，并填入res中，返回字符串长度
int
int2char(char* res, int xx)
{
  static char digits[] = "0123456789";
  char buf[16];
  int i, j, neg;
  uint x;

  neg = 0;
  if(xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    buf[i++] = digits[x % 10];
  }while((x /= 10) != 0);
  if(neg)
    buf[i++] = '-';

  j = 0;
  while(--i >= 0)
    res[j++] = buf[i];

  return j;
}

// 判断chs是否以prefix开头（n个字符），是返回1，否返回0
int
startswidth(uchar *chs, uchar *prefix, int n)
{
  int i;
  for(i = 0; i < n; i++)
    if(chs[i] != prefix[i])
      return 0;
  return 1;
}