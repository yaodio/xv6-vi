#include "vi.h"
#include "vulib.h"
#include "baseline.h"
#include "color.h"

#include "../types.h"
#include "../stat.h"
#include "../user.h"
#include "../fcntl.h"
#include "../console.h"
#include "../kbd.h"

// 多文件共享全局变量不能在头文件里定义
text tx  = {NULL, NULL, NULL};  // 全局文档变量
cursor cur = {0, 0, NULL};      // 全局光标变量

void changecolor(int mode);
void beautify(void);
void printline(int row, line *l);
void printlines(int row, line *l);
int insertc(line *l, int i, uchar c);

/********************光标操作********************/
/*                                             */

// 根据光标所在屏幕的行数，往前推出屏幕上指向的第0行
line*
getfirstline(void)
{
  int row = cur.row;
  line* first = cur.l;

  while(row-- > 0 && first->prev != NULL)
    first = first->prev;
  return first;
}

// 特殊情况处理：光标所在列为MAX_COL
// 出现的原因是，当光标指向的这一行与下一行不同段 且 这一行为MAX_COL个字符
// 此时光标会挤到下一行，但还是指向这一行
void
maxcoldown(void)
{
  int pos = SCREEN_WIDTH * cur.row + cur.col;
  int cc = (DEFAULT_COLOR << 8) | '\0';

  // 光标所在行为倒数第二行（底线行的上一行）需要整个屏幕内容下移一行打印
  if(cur.row == BASE_ROW-1){
    printlines(0, getfirstline()->next);
    pos -= SCREEN_WIDTH;
  }

  // 如果有下一行，则光标处显示下一行的字符
  if(cur.l->next)
    cc = (cur.l->next->colors[0] << 8) | cur.l->next->chs[0];
  setcurpos(pos, cc);
}

// 将光标设置到屏幕上, 确保row在 [0, BASE_ROW]，col在 [0, MAX_COL] 范围内
// col = MAX_COL时要特殊处理
void
showcur(void)
{
  int pos, cc;

  // 特殊情况
  if(cur.col == MAX_COL){
    maxcoldown();
    return;
  }

  // 计算光标的位置 以及 获取该位置的字符
  pos = SCREEN_WIDTH * cur.row + cur.col;
  // 此处 col < MAX_COL，数组访问不会越界
  cc = (cur.l->colors[cur.col] << 8) | cur.l->chs[cur.col];
  setcurpos(pos, cc);
}

// 光标下移, 无法移动则返回0
int
curdown(void)
{
  // 已经是文档最后一行，无法下移
  if(cur.l->next == NULL){
    // 光标已经在最后一行尾部，无需操作
    if(cur.col == cur.l->n)
      return 0;
    // 否则移到行尾
    cur.col = cur.l->n;
  }
  else{
    // 特殊情况时光标指向当前行行尾，但显示位置在下一行行首
    // 那么就修改指针指向下一行行首
    if(cur.col == MAX_COL){
      cur.col = 0;
      cur.row++;
      cur.l = cur.l->next;
    }
      // 光标在当前行中（首）
    else{
      cur.row++;
      cur.l = cur.l->next;
      // 下一行的这个位置没有字符，则左移到最后一个字符的位置
      if(cur.col > cur.l->n)
        cur.col = cur.l->n;
    }

    // 光标移到了底线行，需要整个屏幕内容下移一行打印
    if(cur.row >= BASE_ROW){
      printlines(0, getfirstline()->next);
      cur.row = BASE_ROW - 1;
    }
  }
  showcur();
  return 1;
}

// 光标上移, 无法移动则返回0
int
curup(void)
{
  // 已经是文档首行，无法上移
  if(cur.l->prev == NULL){
    // 光标已经在首部，无需操作
    if(cur.col == 0)
      return 0;
    // 否则移到行首
    cur.col = 0;
  }
  else{
    // 特殊情况时光标指向在当前行行尾（MAX_COL处），但显示位置在下一行行首
    // 那么移到当前行行首
    if(cur.col == MAX_COL)
      cur.col = 0;
      // 光标在行中（首）
    else{
      cur.row--;
      cur.l = cur.l->prev;
      // 上一行的这个位置没有字符，则左移到最后一个字符的位置
      if(cur.col > cur.l->n)
        cur.col = cur.l->n;
    }

    // 需要屏幕上移一行打印
    if(cur.row < 0){
      cur.row = 0;
      printlines(0, cur.l);
    }
  }
  showcur();
  return 1;
}

// 光标左移, 无法移动则返回0
int
curleft(void)
{
  cur.col--;

  // 底线模式第0列为':'
  if(cur.row == BASE_ROW && cur.col < 1) {
    cur.col = 1;
    return 0;
  }

  // 在文档首行
  if(cur.l->prev == NULL){
    // 已经开头，无法左移
    if(cur.col < 0){
      cur.col++;
      return 0;
    }
  }
    // 不是文档首行
  else{
    // 在开头左边，则移到上一行尾部
    if(cur.col < 0){
      cur.row--;
      cur.l = cur.l->prev;
      cur.col = cur.l->n;
      // 上一行是同一段（因此也一定是满的），cur.col移到最后一个字符处（n-1, 同时也是 MAX_COL-1）
      // 否则如果上一行是满的但不同一段，cur.col留在MAX_COL位置（特殊情况）
      if(cur.l->paragraph)
        cur.col--;
    }
  }

  // 需要屏幕上移一行打印
  if(cur.row < 0){
    cur.row = 0;
    printlines(0, cur.l);
  }
  showcur();
  return 1;
}

// 光标右移, 无法移动则返回0
int
curright(void)
{
  int n = cur.l->n;

  cur.col++;

  // 底线模式
  if(cur.row == BASE_ROW){
    if(cur.col > n || cur.col == MAX_COL){
      cur.col--;
      return 0;
    }
    showcur();
    return 1;
  }

  // 在文档最后一行
  if(cur.l->next == NULL){
    // 已经超过尾部，无法右移
    if(cur.col > n){
      cur.col = n;
      return 0;
    }
  }
    // 不是文档的最后一行
  else{
    // 当前行未满（下一行必然不同段）
    if(n < MAX_COL){
      // 可以指向下一行
      if(cur.col > n){
        cur.row++;
        cur.col = 0;
        cur.l = cur.l->next;
      }
    }
      // 当前行已满
    else{
      // col >= n 看下一行是否同段
      if(cur.col >= n){
        // 同段 移到下一行
        if(cur.l->paragraph){
          cur.row++;
          cur.col -= n;
          cur.l = cur.l->next;
        }
          // 不同段 且 col>n 移到下一行（col=n则不需要改变，是特殊情况）
        else if(cur.col > n){
          cur.row++;
          cur.col = 0;
          cur.l = cur.l->next;
        }
      }
    }
  }

  // 光标移到了底线行，需要整个屏幕内容下移一行打印
  if(cur.row >= BASE_ROW){
    printlines(0, getfirstline()->next);
    cur.row = BASE_ROW - 1;
  }
  showcur();
  return 1;
}

// 移动光标至某处
void
curto(int row, int col, line* l) {
  if(col < 0 || col > MAX_COL){
    // TODO: error cur pos
  }
  if(row < 0 || row > BASE_ROW){
    // TODO: error cur pos
  }
  if(row == BASE_ROW && col == MAX_COL){
    // TODO: error cur pos
  }
  cur.row = row; cur.col = col; cur.l = l;
  showcur();
}

/*                                             */
/********************光标操作********************/

// 在屏幕上第row行打印指定的行
void
printline(int row, line *l)
{
  int pos, i;

  pos = row * SCREEN_WIDTH;
  for(i = 0; i < MAX_COL; i++)
    putcc(pos+i, (l->colors[i] << 8) | l->chs[i]);
}

// 从屏幕上第 row 行开始打印指定行 l 以及其后面的行，直到屏幕写满
void
printlines(int row, line *l)
{
  // 保留底线行（最多输出 SCREEN_HEIGHT-1 行）
  while(l != NULL && row < BASE_ROW){
    printline(row++, l);
    l = l->next;
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
    // 下一行被榨干了（变成空行），删掉
    if(l->next->n == 0) {
      deletec(l->next, 0);
      l->paragraph = 0;
    }
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
    printlines(0, getfirstline()->next);
    cur.row = BASE_ROW - 1;
  }else
    printlines(cur.row-1, cur.l->prev);
  showcur();
}
void
insert_tab(line *l)
{
  int tab_length = 4;
  //该行未满直接插入
  if(l->n+tab_length<=SCREEN_WIDTH)
  {
    for(int i = 0;i<tab_length;i++)
      insertc(l,cur.col,' ');
    cur.col+=3;
  }
  else //该行已满
  {
    //如果是最后一行的不会插入tab
    if(cur.row >= SCREEN_HEIGHT-2)
      return;
    else
    {
      if(SCREEN_WIDTH - cur.col < tab_length)//如果光标位置到行尾的距离不足以存放一个tab,tab将会放到下一行
      {
        int lo = l->n-1;
        for(int j = SCREEN_WIDTH - cur.col;j>0;j--)
        {
          insertc(l->next,0,l->chs[lo]);
          l->chs[lo] = '\0';
          lo--;
        }
        for(int i = 0;i<tab_length;i++)
          insertc(l->next,0,' ');
      }
      else
      {
        for(int i = 0;i<tab_length;i++)
          insertc(l,cur.col,' ');
        cur.col+=3;
      }
    }
  }
}
// 在指定行的第i个位置插入字符c，插入模式使用的是默认颜色，因此不用修改字符的颜色（l->colors数组）
int
insertc(line *l, int i, uchar c)
{
  int j;
  uchar chs[1];
  line *newl;

  switch(c){
    case '\t':
      insert_tab(l);
      break;

    default:
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
          newl->next = l->next;
          l->next->prev = newl;
          l->next = newl;
          newl->prev = l;
        }
      }
      break;
  }

  return 1;
}

// 插入模式，按ESC退出
int
insertmode(void)
{
  int edit = 0;
  uchar c;

  // 关闭颜色
  changecolor(UNCOLORED);

  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    switch(c){
      // 方向键上
      case KEY_UP:
        curup();
        break;
        // 方向键下
      case KEY_DN:
        curdown();
        break;
        // 方向键左
      case KEY_LF:
        curleft();
        break;
        // 方向键右
      case KEY_RT:
        curright();
        break;

        // 删除光标处前一个位置的字符
      case KEY_BACKSPACE:
        if(curleft()){
          edit |= deletec(cur.l, cur.col);
          printlines(cur.row, cur.l);
        }
        break;
      case KEY_DEL:
        edit |= deletec(cur.l, cur.col);
        printlines(cur.row, cur.l);
        break;

      case '\n':
        breakline(cur.l, cur.col);
        break;

      default:
        // 在光标处插入字符c
        edit |= insertc(cur.l, cur.col, c);
        // 重新打印该行（以及之后的行）
        if(cur.l->n == MAX_COL && cur.l->paragraph)
          printlines(cur.row, cur.l);
        else
          printline(cur.row, cur.l);
        curright();
        break;
    }
  }

  // 恢复颜色
  changecolor(COLORFUL);

  return edit;
}


// 底线模式
int
baselinemode(void)
{
  int cmdcode = 0;
  cursor oldcur = cur; // 保存光标
  uchar colon[] = ":";
  line* baseline = newlines(colon, 1); // 底线指针
  curto(BASE_ROW, 1, baseline);
  printline(BASE_ROW, baseline);

  uchar c;
  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    if (c == '\n') {
      cmdcode = cmdhandler(baseline);
      break;
    }

    switch (c) {
      // 方向键左和上，光标左移
      case KEY_LF:
      case KEY_UP:
        curleft();
        break;
        // 方向键右和下，光标右移
      case KEY_RT:
      case KEY_DN:
        curright();
        break;
      default:
        insertc(cur.l, cur.col, c);
        printline(cur.row, cur.l);
        curright();
    }
  }
  // 清空底线
  line* blank = newlines(NULL, 0);
  printline(BASE_ROW, blank);
  // 恢复光标
  cur = oldcur;
  showcur();
  free(baseline);
  free(blank);
  return cmdcode;
}

// 主程序（命令模式）
void
editor(void)
{
  int edit = 0;
  uchar c;

  // 打开彩色模式并打印文件的开头 SCREEN_HEIGHT-1 行
  changecolor(COLORFUL);
  printlines(0, tx.head);
  curto(0, 0, tx.head);

  // 不断读取1个字符进行处理
  while(1){
    c = readc();
    switch(c){
      // 在光标所在字符后进入插入模式
      case 'a':
        curright();
        edit |= insertmode();
        break;
        // 在光标所在字符处进入插入模式
      case 'i':
        edit |= insertmode();
        break;

        // 方向键上
      case KEY_UP:
        curup();
        break;
        // 方向键下
      case KEY_DN:
        curdown();
        break;
        // 方向键左
      case KEY_LF:
        curleft();
        break;
        // 方向键右
      case KEY_RT:
        curright();
        break;

      case ':':
        switch(baselinemode()) {
          case QUIT:
            return;
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

  if(edit)
    savefile();
}

// 命令行输入：editor [path]
// 文件路径path为可选参数，若无则在“临时文件”中使用editor
int
main(int argc, char *argv[])
{
  ushort *backup;         // 屏幕字符备份
  int nbytes;             // 屏幕字符备份内容的字节大小

//  printf(2, "tx in main: %d", &tx);
  // 读取文件，并组织成文本结构体，读取异常则退出
  if(readtext(argc > 1 ? argv[1] : NULL, &tx) < 0){
    exit();
  }

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

// 根据文件类型来着色
void
beautify(void)
{
  // TODO
}