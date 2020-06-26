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