#include "vi.h"
#include "vulib.h"
#include "baseline.h"
#include "cursor.h"
#include "color.h"

#include "../stat.h"
#include "../user.h"
#include "../fcntl.h"
#include "../kbd.h"

// 多文件共享全局变量不能在头文件里定义
text tx  = {NULL, NULL, NULL, 0, 0};                  // 全局文档变量
cursor cur = {0, 0, NULL};                            // 全局光标变量
line baseline = {{'\0'}, {'\0'}, 0, NULL, NULL, 0};   // 底线行

void changecolor(int mode);
int insertc(line *l, int i, uchar c);

// 返回指定行往前推的第i个前驱，如果前驱少于i个，则返回最前面的那个前驱
line*
getprevline(line *l, int i)
{
  while(i-- > 0 && l->prev != NULL)
    l = l->prev;
  return l;
}

// 在屏幕上第row行打印指定的行
void
printline(int row, line *l)
{
  int pos, i;

  pos = row * SCREEN_WIDTH;
  for(i = 0; i < MAX_COL; i++)
    putcc(pos+i, paintc(l->chs[i], l->colors[i]));
}

// 从屏幕上第 row 行开始打印指定行 l 以及其后面的行，直到屏幕写满到BASE_ROW-1行
void
printlines(int row, line *l)
{
  line *blank = newlines(NULL, 0);

  while(row < BASE_ROW){
    if(l != NULL){
      printline(row++, l);
      l = l->next;
    }else{
      printline(row++, blank);
    }
  }
}

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

// 删除指定行的第i个字符
// 删除成功返回1, 失败返回0
int
deletec(line *l, int i)
{
  // 超过下标n没有字符可以删除（非法情况）
  if(i > l->n)
    return 0;
  // 此时 i <= l->n

  if(l->paragraph == 0){
    // 删除的是空行
    if(l->n == 0){
      // 文档唯一的1行，此时文档内容为空，无法删除
      if(l->prev == NULL && l->next == NULL)
        return 0;

      // 删除的是文档第一行
      if(l->prev == NULL)
        tx.head = l->next;
      else
        l->prev->next = l->next;
      l->next->prev = l->prev;

      // 删除的是光标指向的行
      if(cur.l == l){
        cur.l = l->next;
        cur.col = 0;
      }
      free(l);
      return 1;
    }
    // 此时 i <= l->n 且 0 < l->n

    // 不涉及下一行的字符移动
    if(i < l->n){
      while(++i < l->n)
        l->chs[i-1] = l->chs[i];
      l->chs[i-1] = '\0';
      l->n--;
      // 没有字符了 且 跟上一行同一段
      if(l->n == 0 && l->prev != NULL && l->prev->paragraph == 1){
        l->prev->paragraph = 0;
        if(cur.l == l){
          cur.row--;
          cur.col = MAX_COL;
          cur.l = l->prev;
        }
        deletec(l, 0);
      }
      return 1;
    }
    // 此时 0 < i == l->n

    // 文档最后一个字符之后，无法删除
    if(l->next == NULL)
      return 0;

    if(l->n == MAX_COL){
      // 下一行是空行，删掉
      if(l->next->n == 0)
        deletec(l->next, 0);
        // 否则下一行变为同一段
      else
        l->paragraph = 1;

      if(cur.l == l && l->next != NULL){
        cur.l = l->next;
        cur.col = 0;
        cur.row++;
      }
      return 1;
    }
    // 此时 0 < i == l->n < MAX_COL(80), 即该行还有空位
    // 把下一行部分字符挪上来
    while(l->n < MAX_COL && l->next->n > 0){
      l->chs[l->n++] = l->next->chs[0];
      deletec(l->next, 0);
    }
    // 下一行被榨干了（变成空行），删掉
    if(l->next->n == 0)
      deletec(l->next, 0);
      // 否则下一行变为同一段
    else
      l->paragraph = 1;
  }
  else{
    // 同段时光标col（传入的i）不会等于MAX_COL
    // 此时 i < l->n == MAX_COL
    while(++i < l->n)
      l->chs[i-1] = l->chs[i];
    l->chs[i-1] = l->next->chs[0];
    deletec(l->next, 0);
  }

  return 1;
}

// 在指定行的第i个字符处换行
void
breakline(line *l, int i)
{
  int j;
  line *newl;

  // 当前行的下一行属于同一段
  if(l->paragraph){
    // 可能与上一段同行，去掉段落标记即可
    if(i == 0 && l->prev != NULL && l->prev->paragraph)
      l->prev->paragraph = 0;
    else{
      // 将i以及后面的所有字符插入到下一行行首
      for(j = l->n-1; j>=i; j--){
        insertc(l->next, 0, l->chs[j]);
        l->chs[j] = '\0';
        l->n--;
      }
      l->paragraph = 0;
      cur.row++;
      cur.col = 0;
      cur.l = l->next;
    }
  }
    //如果下一行属于另一段，则将i以及后面的所有字符插入到新一行.
  else{
    newl = newlines(l->chs+i, l->n-i);
    memset(l->chs+i, '\0', l->n-i);
    l->n = i;
    if(l->next != NULL){
      l->next->prev = newl;
      newl->next = l->next;
    }
    newl->prev = l;
    l->next = newl;

    cur.row++;
    cur.col = 0;
    cur.l = l->next;
  }

  // 光标移到了底线行，需要整个屏幕内容下移一行打印
  if(cur.row >= BASE_ROW){
    printlines(0, getprevline(cur.l, cur.row)->next);
    cur.row = BASE_ROW - 1;
  }else
    printlines(cur.row-1, cur.l->prev);
  showcur(&cur);
}

// 在指定行的第i个位置插入字符c，插入模式使用的是默认颜色，因此不用修改字符的颜色（l->colors数组）
int
insertc(line *l, int i, uchar c)
{
  int j;
  uchar chs[1];
  line *newl;

  // 该行未满（此时i不可能为MAX_COL）
  if(l->n < MAX_COL){
    for(j = l->n; j > i; j--)
      l->chs[j] = l->chs[j-1];
    l->chs[i] = c;
    l->n++;
  }
    // 该行已满（此时i可能为MAX_COL）
  else{
    // i不为MAX_COL时，保存该行最后1个字符，放到下一行行首
    if(i < MAX_COL){
      chs[0] = l->chs[MAX_COL-1];
      for(j = MAX_COL-1; j > i; j--)
        l->chs[j] = l->chs[j-1];
      l->chs[i] = c;
    }
      // 否则c放到下一行行首
    else
      chs[0] = c;

    // 下一行是同一段
    if(l->paragraph)
      insertc(l->next, 0, chs[0]);
      // 下一行不是同一段，插入新行
    else{
      l->paragraph = 1;
      newl = newlines(chs, 1);
      newl->prev = l;
      newl->next = l->next;
      if(l->next)
        l->next->prev = newl;
      l->next = newl;
    }
  }

  return 1;
}

// 插入模式，按ESC退出
int
insertmode(void)
{
  int edit = 0;
  int tab;
  uchar c;

  // 关闭颜色
  changecolor(UNCOLORED);
  printlines(0, getprevline(cur.l, cur.row));
  showinsertmsg();
  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    switch(c){
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

      // 删除光标处前一个位置的字符
      case KEY_BACKSPACE:
        if(curleft(&cur)){
          edit |= deletec(cur.l, cur.col);
          printlines(cur.row, cur.l);
        }
        break;
      case KEY_DEL:
        edit |= deletec(cur.l, cur.col);
        tx.word_count--;
        printlines(cur.row, cur.l);
        break;

      case '\n':
        breakline(cur.l, cur.col);
        break;

      case '\t':
        tab = TAB_WIDTH;
        while(tab--){
          // 在光标处插入字符c
          edit |= insertc(cur.l, cur.col, ' ');
          tx.word_count++;
        }
        // 重新打印该行（以及之后的行）
        if(cur.l->n == MAX_COL && cur.l->paragraph)
          printlines(cur.row, cur.l);
        else
          printline(cur.row, cur.l);

        tab = TAB_WIDTH;
        while(tab--)
          curright(&cur);
        break;

      default:
        // 在光标处插入字符c
        edit |= insertc(cur.l, cur.col, c);
        // 重新打印该行（以及之后的行）
        if(cur.l->n == MAX_COL && cur.l->paragraph)
          printlines(cur.row, cur.l);
        else
          printline(cur.row, cur.l);
        tx.word_count++;
        curright(&cur);
        break;
    }
  
  showinsertmsg();
  }

  // 恢复颜色
  changecolor(COLORFUL);
  printlines(0, getprevline(cur.l, cur.row));
  return edit;
}

// 底线模式
int
baselinemode(int edit)
{
  int code = NOCHANGE;
  cursor oldcur = cur; // 保存光标
  setline(&baseline, ":", 1, CMD_COLOR);

  curto(&cur, BASE_ROW, 1, &baseline);
  printline(BASE_ROW, &baseline);

  uchar c;
  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    // 按下换行，处理命令
    if (c == '\n'){
      code = baselinehandler(&baseline, edit);
      break;
    }
    // 没有字符了且按下了退格
    if ((c == KEY_BACKSPACE && !curleft(&cur))) 
      break; 

    switch (c) {
      // 方向键左和上，光标左移
      case KEY_LF:
      case KEY_UP:
        curleft(&cur);
        break;
        // 方向键右和下，光标右移
      case KEY_RT:
      case KEY_DN:
        curright(&cur);
        break;
      // 删除光标处前一个位置的字符
      case KEY_BACKSPACE:
        deletec(cur.l, cur.col);
        printline(cur.row, cur.l);
        break;
      default:
        insertc(cur.l, cur.col, c);
        printline(cur.row, cur.l);
        curright(&cur);
    }
  }

  // 恢复光标
  cur = oldcur;
  showcur(&cur);

  return code;
}

// 主程序（命令模式）
void
editor(void)
{
//  printf(1, "editor begin\n");
  int edit = 0;
  uchar c;

  // 打开彩色模式并打印文件的开头 SCREEN_HEIGHT-1 行
  changecolor(COLORFUL);
  printlines(0, tx.head);
  curto(&cur, 0, 0, tx.head);

  // 不断读取1个字符进行处理
  while(1){
    showpathmsg();
    c = readc();
    switch(c){
      // 在光标所在字符后进入插入模式
      case 'a':
        curright(&cur);
        edit |= insertmode();
        break;
        // 在光标所在字符处进入插入模式
      case 'i':
        edit |= insertmode();
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
            printline(BASE_ROW, &baseline);
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

// 命令行输入：editor [path]
// 文件路径path为可选参数，若无则在“临时文件”中使用editor
int
main(int argc, char *argv[])
{
  ushort *backup;         // 屏幕字符备份
  int nbytes;             // 屏幕字符备份内容的字节大小
//  reg_test();
//  hashmap_test();

//  printf(2, "tx in main: %d", &tx);
  // 读取文件，并组织成文本结构体，读取异常则退出
  if(readtext(argc > 1 ? argv[1] : NULL, &tx) < 0){
    exit();
  }
  read_syntax();
//  printf(1, "read end\n");
//  printf(1, "beautify end\n");

//  while(1) ;

  // 备份屏幕上的所有字符
  nbytes = getcurpos() * sizeof(backup[0]);
  if((backup = (ushort*)malloc(nbytes)) == NULL){
    printf(2, "editor: cannot allocate memory to backup characters on the screen\n");
    exit();
  }
  bks(backup, nbytes);

  // 清屏，关闭控制台的flag，然后进入编辑器
  cls();
  consflag(0, 0, 0);
  editor();

  // 退出编辑器，开启控制台的flag，并还原屏幕上的所有字符
  consflag(1, 1, 1);
  rcs(backup, nbytes);
  free(backup);
  // TODO: free pointers in tx.
  exit();
}

// 切换颜色模式
void
changecolor(int mode)
{
  line *tmp;

  switch(mode){
    case COLORFUL:
      // 彩色模式根据文件类型来着色
      beautify();
      break;

    case UNCOLORED:
    default:
      // 使用默认颜色
      tmp = tx.head;
      while(tmp){
        memset(tmp->colors, DEFAULT_COLOR, MAX_COL);
        tmp = tmp->next;
      }
      break;
  }
}