#include "vi.h"

// 多文件共享全局变量不能在头文件里定义
text    tx        = {NULL, NULL, 0, 0};                   // 全局文档变量
cursor  cur       = {0, 0, NULL};                         // 全局光标变量
line    baseline  = {{'\0'}, {'\0'}, 0, NULL, NULL, 0};   // 底线行

static ushort *backup;    // 屏幕字符备份
static int nbytes;        // 屏幕字符备份内容的字节大小

// 主程序（命令模式）
void
vi(void)
{
//  printf(1, "vi begin\n");
  int edit = 0;
  uchar c;

  if (strlen(tx.path) <= 0) 
    help();

  // 打印文件的开头 SCREEN_HEIGHT-1 行
  printlines(0, tx.head, 1);
  curto(&cur, 0, 0, tx.head);

  // 不断读取1个字符进行处理
  while(1){
    showpathmsg();
    c = readc();
    switch(c){
      // 在光标所在字符后进入编辑模式
      case 'a':
        curright(&cur);
        edit |= editmode();
        break;
        // 在光标所在字符处进入编辑模式
      case 'i':
        edit |= editmode();
        break;

        // 方向键上
      case KEY_UP: 
        curup(&cur);
        break;
        // 方向键下
      case KEY_DN:
        curdown(&cur);
        break;
        // 方向键左
      case KEY_LF:
        curleft(&cur);
        break;
        // 方向键右
      case KEY_RT:
        curright(&cur);
        break;

      case ':':
        switch(baselinemode(edit)) {
          case SAVE:
            edit = 0;
            break;
          case QUIT:
            return;
          case ERROR:
            printline(BASE_ROW, &baseline, 0);
            c = readc();
            break;
          default:
            break;
        }
        break;

        // case 'e' 只是调试用的
      case 'e':
        return;

        // TODO: 添加其他case
      default:
        break;
    }
  }
}

// 释放所有申请的空间
void
freeall(void)
{
  free(backup);
  freetx(&tx);
  freehelptx();
  // TODO: 检查是否还有未释放的内存，添加到这里
}

// 命令行输入：vi [path]
// 文件路径path为可选参数，若无则在“临时文件”中使用vi
int
main(int argc, char *argv[])
{
  // 读取文件，并组织成文本结构体，读取异常则退出
  if(readtext(argc > 1 ? argv[1] : NULL, &tx) < 0)
    exit();
  // 根据文件类型准备正则规则
  read_syntax();

  // 备份屏幕上的所有字符
  nbytes = getcurpos() * sizeof(backup[0]);
  if((backup = (ushort*)malloc(nbytes)) == NULL){
    printf(2, "vi: cannot allocate memory to backup characters on the screen\n");
    exit();
  }
  bks(backup, nbytes);

  // 清屏，关闭控制台的flag，然后进入编辑器
  cls();
  consflag(0, 0, 0);

  vi();

  // 退出编辑器，开启控制台的flag，并还原屏幕上的所有字符，释放内存
  consflag(1, 1, 1);
  rcs(backup, nbytes);
  freeall();
  exit();
}
