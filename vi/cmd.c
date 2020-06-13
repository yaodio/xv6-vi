#include "vi.h"
#include "cmd.h"
#include "vulib.h"
#include "../types.h"
#include "../user.h"

extern struct text tx;

int
cmdhandler(line* cmd)
{
  if (compare(cmd->chs, ":q")) {
    return QUIT;
  } else if (compare(cmd->chs, ":w")) {
    savefile();
  } else if (compare(cmd->chs, ":wq")) {
    savefile();
    return QUIT;
  }
    // TODO: 添加其他命令
  else {
    // ERROR
  }
  return NORMAL;
}

void
savefile(void)
{
  if (!(tx.path)) {
    // TODO: 输入保存路径
//    printf(2, "tx in cmd: %d, no path\n", &tx);
  }
  else writetext(tx.path, tx.head);
}