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
text tx  = {NULL, NULL, NULL};  // 全局文档变量
cursor cur = {0, 0, NULL};      // 全局光标变量

void changecolor(int mode);
void beautify(void);
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
      if(l->n == 0 && l->prev != NULL && l->prev->paragraph == 1){
        l->prev->paragraph = 0;
        cur.row--;
        cur.col = MAX_COL;
        cur.l = l->prev;
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
    printlines(0, getprevline(cur.l, cur.row)->next);
    cur.row = BASE_ROW - 1;
  }else
    printlines(cur.row-1, cur.l->prev);
  showcur(&cur);
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
          newl->prev = l;
          newl->next = l->next;
          if(l->next)
            l->next->prev = newl;
          l->next = newl;
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
  printlines(0, getprevline(cur.l, cur.row));
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
        curright(&cur);
        break;
    }
  }

  // 恢复颜色
  changecolor(COLORFUL);
  printlines(0, getprevline(cur.l, cur.row));
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
  curto(&cur, BASE_ROW, 1, baseline);
  printline(BASE_ROW, baseline);

  uchar c;
  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    if (c == '\n') {
      cmdcode = cmdhandler(baseline); // 按下换行，处理命令
      break;
    }
    if ((c == KEY_BACKSPACE && !curleft(&cur))) break; // 没有字符了且按下了退格

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
  line* baseline_tip;
  switch (cmdcode) {
  case ERROR:
    baseline_tip = newlines("ERROR!", 6);
    paintl(baseline_tip, WHITE, RED);
    break;
  default:
    baseline_tip = newlines(NULL, 0); // 清空底线
    break;
  }
  printline(BASE_ROW, baseline_tip);
  // 恢复光标
  cur = oldcur;
  showcur(&cur);
  free(baseline);
  free(baseline_tip);
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
  curto(&cur, 0, 0, tx.head);

  // 不断读取1个字符进行处理
  while(1){
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
int//匹配两个字符串
matching(uchar str1[],int start_pos1,uchar str2[],int start_pos2,int cpnum)
{
    int i;
    for(i=0;i<cpnum;i++)
        if(str1[start_pos1+i] != str2[start_pos2+i])   
            return  0;
    return 1;
}
void
beautify_cfile()
{
    uchar c_wordslib[15][10] = {"main",
                               "printf",
                               "exit",
                               "#include",
                               "for",
                               "if",
                               "break",
                               "int",
                               "char",
                               "float",
                               "double"
                               "=",
                               ">",
                               "<"
                               "*"};
    uchar c_colorlib[15] = {(BLACK << 4) | GREEN,
                           (BLACK << 4) | GREEN,
                           (BLACK << 4) | GREEN,
                           (BLACK << 4) | RED,
                           (BLACK << 4) | RED,
                           (BLACK << 4) | RED,
                           (BLACK << 4) | RED,
                           (BLACK << 4) | YELLOW,
                           (BLACK << 4) | YELLOW,
                           (BLACK << 4) | YELLOW,
                           (BLACK << 4) | YELLOW,
                           (BLACK << 4) | LIGHT_CYAN,
                           (BLACK << 4) | LIGHT_CYAN,
                           (BLACK << 4) | LIGHT_CYAN,
                           (BLACK << 4) | LIGHT_CYAN};
    int c_lenthlib[15] = {4,6,4,8,3,2,5,3,4,5,6,1,1,1,1};
    int words_num = 15;
    line *temp = tx.head;
    while(1)
    {
        int i;
        for(i=0;i<temp->n;i++)
        {
            if(i==0)//查询词库并着色
            {
                int k;
                for(k=0;k<words_num;k++)
                {
                    if(matching(temp->chs,0,c_wordslib[k],0,c_lenthlib[k]))
                    {
                        int j;
                        for(j=0;j<c_lenthlib[k];j++)
                          temp->colors[j] = c_colorlib[k];
                        i+=c_lenthlib[k];
                    }
                }
            }
            else if(temp->chs[i] == ' '||temp->chs[i] == '('||temp->chs[i] == ',')//查询词库并着色
            {
                int k;
                for(k=0;k<words_num;k++)
                {
                    if(matching(temp->chs,i+1,c_wordslib[k],0,c_lenthlib[k]))
                    {
                        int j;
                        for(j=0;j<c_lenthlib[k];j++)
                            temp->colors[i+1+j] = c_colorlib[k];
                        i+=c_lenthlib[k]+1;
                    }
                }
            } //在词库中添加了运算符这种情况，但是有时候没有效果，所以暂时把运算符单独拿出来处理
            else if(temp->chs[i] == '*'||temp->chs[i] == '+'||temp->chs[i] == '-'||temp->chs[i] == '='||temp->chs[i] == '/')
              temp->colors[i] = (BLACK << 4) | LIGHT_CYAN;
            else
              temp->colors[i] = (BLACK << 4) | WHITE;//把其他字符变为白色
        }
        if(temp == tx.tail)
            break;
        temp = temp->next;
    }
}
// 根据文件类型来着色
void
beautify(void)
{
  int i,temp;
  for(i=0;i<sizeof(tx.path);i++)
    if(tx.path[i] == '.')
    {
      temp = i;
      break;
    }
  if(matching(tx.path,temp,".c",0,2)) //如果是C文件
    beautify_cfile();
}