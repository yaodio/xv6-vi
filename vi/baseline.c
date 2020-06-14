#include "vi.h"
#include "baseline.h"
#include "vulib.h"
#include "../types.h"
#include "../user.h"

extern struct text tx;

int
cmdhandler(line* cmd)
{
  if (strcmp(cmd->chs, ":q") == 0) {
    return QUIT;
  } else if (strcmp(cmd->chs, ":w") == 0) {
    savefile();
  } else if (strcmp(cmd->chs, ":wq") == 0) {
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