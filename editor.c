#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define NULL 0  // 空指针

// 从指定的文件描述符中读取所有行，调用者负责文件描述符的打开和关闭
char**
readlines(int fd)
{
  char **lines = NULL;
  // TODO: 读取所有行

  return lines;
}

// 打印所有行
void
printlines(char **lines)
{
  // TODO: check NULL and print
}

// 从标准输入（键盘）读入1个字符
char
readc()
{
  static char buf[1];

  while (1){
    if(read(0, buf, 1) > 0 && buf[0] != 0)
      return buf[0];
  }
}

void
wirtetopath(char **lines, char *savepath)
{
  // TODO: 输出到指定路径，若路径不存在，则提示输入保存路径，或者直接退出不保存
}

void
editor(char **lines, char *savepath)
{
  int editflag = 0;
  char c;
  // TODO: 核心程序
  printlines(lines);
  // 调试代码
  // printf(1, "editor: %s\n", savepath);
  // printf(1, "curpos:%d\n", getcurpos());
  // sleep(500);
  // 调试结束

  // 不断读取1个字符进行处理
  while(1){
    c = readc();
    switch(c){
    case 'i':
      // TODO: 进入编辑模式
      printf(1,"i!");
      break;
    
    // TODO: 添加其他case
    default:
      break;
    }
  }

    if(editflag)
      wirtetopath(lines, savepath);
  }

  // 命令行输入：editor [path]
  // 文件路径path为可选参数，若无则在“临时文件”中使用editor
  int
  main(int argc, char *argv[])
  {
    char *path = NULL;      // 文件路径
    char **lines = NULL;    // 文件内容
    int fd;                 // 文件描述符
    struct stat st;         // 文件信息
    ushort *backup;         // 屏幕字符备份
    int nbytes;             // 屏幕字符备份内容的字节大小

    // 备份屏幕上的所有字符
    nbytes = getcurpos() * sizeof(backup[0]);
    if((backup = (ushort*)malloc(nbytes)) == NULL){
      printf(2, "editor: cannot allocate memory to backup characters on the screen\n");
      exit();
    }
    bks(backup, nbytes);

    // 输入了文件路径
    if(argc > 1){
      path = argv[1];
      // 路径存在可被打开
      if((fd = open(path, O_RDONLY)) >= 0){
        // 获取文件信息失败则退出
        if(fstat(fd, &st) < 0){
          printf(2, "editor: cannot stat %s\n", path);
          close(fd); // 与open匹配
          exit();
        }

        // 否则检查文件信息，是否为文件（可能是目录）
        if(st.type != T_FILE){
          printf(2, "editor: cannot edit a directory: %s\n", path);
          close(fd); // 与open匹配
          exit();
        }

        // 走到这里说明成功打开了一个文件，读取其中的所有行
        lines = readlines(fd);
        close(fd); // 与open匹配
      }
    }

    // 清屏，关闭控制台的flag，然后进入编辑器
    cls();
    consflag(0,0);
    editor(lines, path);

    // 退出编辑器，开启控制台的flag，并还原屏幕上的所有字符
    consflag(1,1);
    rcs(backup, nbytes);
    free(backup);
    // TODO: free **lines
    exit();
  }