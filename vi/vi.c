#include "vi.h"

// 多文件共享全局变量不能在头文件里定义
text    tx        = {NULL, NULL, 0, 0};                   // 全局文档变量
cursor  cur       = {0, 0, NULL};                         // 全局光标变量
line    baseline  = {{'\0'}, {'\0'}, 0, NULL, NULL, 0};   // 底线行

static ushort *backup;    // 屏幕字符备份
static int nbytes;        // 屏幕字符备份内容的字节大小

// 预览模式
void
viewmode(void)
{
  int edit = 0;
  uchar c;

  // 不断读取1个字符进行处理
  while(1){
    showpathmsg();
    c = readc();
    switch(c){
      /**         方向键操作         **/
      case KEY_UP: curup(&cur);    break;  // 方向键上
      case KEY_DN: curdown(&cur);  break;  // 方向键下
      case KEY_LF: curleft(&cur);  break;  // 方向键左
      case KEY_RT: curright(&cur); break;  // 方向键右

      /**    进入编辑模式的2种途径     **/
      case 'a': curright(&cur);            // 在光标所在字符后进入编辑模式
      case 'i': edit |= editmode(); break; // 在光标所在字符处进入编辑模式

      /**         进入底线模式        **/
      case ':':
        switch(baselinemode(edit)) {
          // 从help函数返回
          case HELP:
            printlines(0, getprevline(cur.l, cur.row), 1);
            showcur(&cur);
            break;
          // 保存了文件返回
          case SAVE:
            edit = 0;
            break;
          // 需要退出程序
          case QUIT:
            return;
          // 异常命令
          case ERROR:
            printline(BASE_ROW, &baseline, 0);  // 在底线行打印异常信息
            c = readc(); // 阻塞一下程序给用户确认异常信息，输入任意字符即可退出阻塞
            break;
          default:
            break;
        }
        break;

      /**         常用功能键         **/
      // 移动到行首
      case '0':
        curto(&cur, cur.row, 0, cur.l);
        break;
      // 移动到行尾
      case '$':
        curto(&cur, cur.row, cur.l->n, cur.l);
        break;
      // 移到屏幕第一行的行首
      case 'H':
        curto(&cur, 0, 0, getprevline(cur.l, cur.row));
        break;
      // 移到底线行上一行的行首
      case 'L':
        curto(&cur, BASE_ROW-1, 0, getnextline(cur.l, BASE_ROW-1-cur.row));
        break;
      // 移动到文档末尾
      case 'G':
        while(curdown(&cur))
          ;
        break;
      
      // TODO: 添加其他case实现其他功能
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
  /**      文件准备阶段      **/
  // 读取文件，并组织成文本结构体，读取异常则退出
  if(readtext(argc > 1 ? argv[1] : NULL, &tx) < 0)
    exit();
  // 根据文件类型准备正则规则
  read_syntax();

  /**      屏幕清理阶段      **/
  nbytes = getcurpos() * sizeof(backup[0]);
  if((backup = (ushort*)malloc(nbytes)) == NULL){
    printf(2, "vi: cannot allocate memory to backup characters on the screen\n");
    exit();
  }
  bks(backup, nbytes);  // 备份屏幕上的所有字符
  cls();                // 清屏
  consflag(0, 0, 0);    // 关闭控制台的flag

  /**      程序运行阶段      **/
  // 无参启动，显示帮助页
  if (argc <= 1)
    help();
  // 打印文件的开头 SCREEN_HEIGHT-1 行
  printlines(0, tx.head, 1);
  // 光标移动到左上角，指向第一行行首
  curto(&cur, 0, 0, tx.head);
  // 进入vi编辑器（预览模式）
  viewmode();

  /**      程序收尾阶段      **/
  consflag(1, 1, 1);    // 开启控制台的flag
  rcs(backup, nbytes);  // 还原屏幕上的所有字符
  freeall();            // 释放内存

  exit();
}
