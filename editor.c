#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "console.h"
#include "color.h"
#include "editor.h"
#include "kbd.h"

void changecolor(int mode);
void beautify(void);
void printline(int row, line *l);
void printlines(int row, line *l);

char
compare(char* a, const char* b)
{
  int i = 0;
  while (a[i] != '\0' && b[i] != '\0' && a[i] == b[i])
    i++;
  return (a[i] == '\0' && b[i] == '\0');
}

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
  if(cur.row == SCREEN_HEIGHT-2){
    printlines(0, getfirstline()->next);
    pos -= SCREEN_WIDTH;
  }

  // 如果有下一行，则光标处显示下一行的字符
  if(cur.l->next)
    cc = (cur.l->next->colors[0] << 8) | cur.l->next->chs[0];
  setcurpos(pos, cc);
}

// 将光标设置到屏幕上, 确保row在 [0, SCREEN_HEIGHT - 2]，col在 [0, MAX_COL] 范围内
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

// 光标下移
void
curdown(void)
{
  // 已经是文档最后一行，无法下移
  if(cur.l->next == NULL){
    // 光标已经在最后一行尾部，无需操作
    if(cur.col == cur.l->n)
      return;
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
    if(cur.row >= SCREEN_HEIGHT - 1){
      printlines(0, getfirstline()->next);
      cur.row = SCREEN_HEIGHT - 2;
    }
  }
  showcur();
}

// 光标上移
void
curup(void)
{
  // 已经是文档首行，无法上移
  if(cur.l->prev == NULL){
    // 光标已经在首部，无需操作
    if(cur.col == 0)
      return;
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
}

// 光标左移
void
curleft(void)
{
  cur.col--;

  // 在文档首行
  if(cur.l->prev == NULL){
    // 已经开头，无法左移
    if(cur.col < 0 || (mode == CMD_MODE && cur.col < 1))
      cur.col++;
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
}

// 光标右移
void
curright(void)
{
  int n = cur.l->n;

  cur.col++;

  // 在文档最后一行
  if(cur.l->next == NULL){
    // 已经超过尾部，无法右移
    if(cur.col > n)
      cur.col--;
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
  if(cur.row >= SCREEN_HEIGHT - 1 && mode != CMD_MODE){
    printlines(0, getfirstline()->next);
    cur.row = SCREEN_HEIGHT - 2;
  }
  showcur();
}

// 移动光标至某处
void
curto(int row, int col, line* l) {
  if (row < SCREEN_HEIGHT && col < SCREEN_WIDTH) {
    cur.row = row; cur.col = col; cur.l = l;
    showcur();
  }
}

/*                                             */
/********************光标操作********************/

// 根据传入的字符数组，构造双向链表，每个节点是一行
line*
newlines(uchar *chs, uint n)
{
  int i;
  line *l = (line*)malloc(sizeof(line));
  memset(l->chs, '\0', MAX_COL);
  memset(l->colors, DEFAULT_COLOR, MAX_COL);
  l->n = 0;
  l->paragraph = 0;
  l->prev = l->next = NULL;
  
  // 空行
  if(chs == NULL || n == 0)
    return l;

  for(i = 0; i < n; i++){
    // 换行
    if(chs[i] == '\n'){
      l->next = newlines(chs+i+1, n-i-1);
      l->next->prev = l;
      break;
    }
    // 写满一行，则下一行与该行同段
    if(i >= MAX_COL){
      l->next = newlines(chs+i, n-i);
      l->next->prev = l;
      l->paragraph = 1;
      break;
    }
    // 填写一行中
    l->chs[i] = chs[i];
    l->n++;
  }
  
  return l;
}

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
  while(l != NULL && row < SCREEN_HEIGHT - 1){
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

void
savefile(void)
{
  if (!(tx.path)) {
    // TODO: 输入保存路径
  }
  else writeto(tx.path, tx.head);
}

void
writeto (char* path, line* head)
{
  // TODO: 输出到path
}
void
insert_linebreak(line *l)
{
  //如果当前行文本最后一行，则不会执行换行
  if(cur.row >= SCREEN_HEIGHT-2)
    return;
  //如果当前行的下一行属于同一段，则将pos后面的所有字符插入到下一行行首
  if(l->paragraph)
  {
    int char_num = 0;
    for(int j = l->n-1;j>cur.col;j--)
    {
      insertc(l->next,0,l->chs[j]);
      l->chs[j] = '\0';
      char_num++;
    }
  }//如果下一行属于另一段，则将pos后面的所有字符插入到新一行.
  else
  {
    uint cnum = l->n - cur.col;
    uchar *temp = (uchar*)malloc(cnum);
    for(int i = 0;i<cnum;i++)
      temp[i] = l->chs[i+cur.col];
    line *new_l = newlines(temp,cnum);
    l->next->prev = new_l;
    new_l->next = l->next;
    new_l->prev = l;
    l->next = new_l;
  }
// 在指定行的第i个位置插入字符c，插入模式使用的是默认颜色，因此不用修改字符的颜色（l->colors数组）
void
insertc(line *l, int i, uchar c)
{
  int j;
  uchar chs[1];
  line *newl;

  switch(c){
  case '\n':
    insert_linebreak(l);
    break;

  case '\t':
    // TODO
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

    default:
      // 在光标处插入字符c
      insertc(cur.l, cur.col, c);
      edit = 1;
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

/**
 * 底线模式
 */
int
commandline_mode(void)
{
  mode = CMD_MODE; // 模式切换
  int cmdcode = 0;
  struct cursor cur_old = cur; // 保存光标
  char colon[] = ":";
  line* baseline = newlines(colon, 1); // 底线指针
  basehead = SCREEN_HEIGHT-1; // 底线首行
  curto(basehead, 1, baseline); // 移动光标
  printline(basehead, baseline);
  uchar c;
  while (1) {
    c = readc();
    if (c == '\n') {
      cmdcode = cmdhandler(baseline);
      break;
    }
    switch (c) {
    case KEY_UP:
    case KEY_DN:
      break; // 禁止上下
    case KEY_LF: // 方向键左
      curleft();
      break;
    case KEY_RT: // 方向键右
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
  printline(SCREEN_HEIGHT-1, blank);
  // 恢复光标
  cur = cur_old;
  showcur();
  free(baseline);
  return cmdcode;
}

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
    return NORMAL;
  }
}

// 主程序
void
editor(void)
{
  int edit = 0;
  int pos;
  uchar c;

  // 打开彩色模式并打印文件的开头 SCREEN_HEIGHT-1 行
  changecolor(COLORFUL);
  printlines(0, tx.head);
  cur.l = tx.head;
  showcur();
  // 调试代码
  // printf(1, "editor: %s\n", savepath);
  // printf(1, "curpos:%d\n", getcurpos());
  // sleep(500);
  // 调试结束

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

    // ESC
    case KEY_ESC:
      // TODO
      printf(1,"ESC");
      break;
    case ':':
      switch(commandline_mode()) {
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

// 从指定的文件路径中读取所有内容，并组织成行结构（双向链表），出错时返回-1
int
readtext(char *path)
{
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  uint nbytes;            // 文件大小（字节数）
  uchar *chs;             // 文件中的所有字符

  // 路径存在且可被打开
  if(path != NULL && (fd = open(path, O_RDONLY)) >= 0){
    // 获取文件信息失败则退出
    if(fstat(fd, &st) < 0){
      printf(2, "editor: cannot stat %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 否则检查文件信息，是否为文件（可能是目录）
    if(st.type != T_FILE){
      printf(2, "editor: cannot edit a directory: %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 走到这里说明成功打开了一个文件，读取其中的所有字符
    nbytes = st.size;
    chs = (uchar*)malloc(nbytes);
    read(fd, chs, nbytes);
    close(fd); // 与open匹配
  }
  // 路径不存在
  else{
    nbytes = 0;
    chs = NULL;
  }

  tx.path = path;
  // 将文件内容组织成行结构，并用指针进行标记
  tx.head = tx.tail = newlines(chs, nbytes);

  // 定位尾行
  while(tx.tail->next != NULL){
    tx.tail = tx.tail->next;
  }

  return 0;  
}

// 命令行输入：editor [path]
// 文件路径path为可选参数，若无则在“临时文件”中使用editor
int
main(int argc, char *argv[])
{
  ushort *backup;         // 屏幕字符备份
  int nbytes;             // 屏幕字符备份内容的字节大小

  // 读取文件，并组织成文本结构体，读取异常则退出
  if(readtext(argc > 1 ? argv[1] : NULL) < 0){
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
  consflag(0,0);
  editor();

  // 退出编辑器，开启控制台的flag，并还原屏幕上的所有字符
  consflag(1,1);
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