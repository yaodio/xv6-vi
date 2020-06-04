#include "types.h"
#include "stat.h"
#include "user.h"

#define NULL 0  // 空指针

void
editor(char *path) 
{
  // TODO
  printf(1, "editor: %s\n", path);
}

int
main(int argc, char *argv[])
{
  // 命令行输入：editor [path]
  // 文件路径path为可选参数，若无则在“临时文件”中使用editor
  char *path = NULL;
  if(argc > 1)
    path = argv[1];

  editor(path);
  exit();
}