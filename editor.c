#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "console.h"
#include "color.h"
#include "editor.h"

// 根据传入的字符数组，构造双向链表，每个节点是一行
line*
newlines(char *chs, uint n)
{
  int i;
  line *l = (line*)malloc(sizeof(line));
  memset(l->chs, '\0' | DEFAULT_COLOR, MAX_COL);
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
    l->chs[i] = chs[i] | DEFAULT_COLOR;
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
    putcc(pos+i, l->chs[i]);
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

// TODO：这个函数还没有测试过，等支持处理方向键后再测试
// 获取在光标pos位置上的字符
char
getcatpos(int pos)
{
  int row, col;
  line *tmp;
  // TODO：检查pos的值，防止越界（可能第row行不存在，超出了双向链表范围）

  // pos = row * SCREEN_WIDTH + col
  row = pos / SCREEN_WIDTH; // 光标在屏幕的第row行
  col = pos % SCREEN_WIDTH; // 光标在屏幕的第col行

  // 从屏幕第0行开始，找到第row行
  tmp = tx.show;
  while(row--){
    tmp = tmp->next;
  }
  // 返回第row行的第col个字符
  return tmp->chs[col];
}

void
wirtetopath(void)
{
  // TODO: 输出到tx->path，若路径不存在，则提示输入保存路径，或者直接退出不保存
}

// 在指定行的第i个位置插入字符c
void
insertc(line *l, int i, char c)
{
  if(l->n<SCREEN_WIDTH)//if line is not full
  {
    for(int j=l->n+1;j>i;j--)
      l->chs[j] = l->chs[j-1];
    l->chs[i] = c;
    l->n++;
  }
  else
  {
    char last_char = l->chs[SCREEN_WIDTH-1];//put the last char at the beginning of nest line
    for(int j=SCREEN_WIDTH-1;j>i;j--)
      l->chs[j] = l->chs[j-1];
      l->chs[i] = c;
    if(l->paragraph)
      insertc(l->next,0,last_char);
    else
    {
      line *new_l = (line*)malloc(sizeof(line));
      new_l->n = 1;
      new_l->paragraph = 0;
      new_l->chs[0] = last_char;
      new_l->chs[1] = '\0';
      
      line *third_line = l->next;
      l->next = new_l;
      new_l->next = third_line;
      third_line->prev = new_l;
      new_l->prev = l;
      l->paragraph = 1;
    }
  }
}

void 
insert(void)
{
  int pos = getcurpos();
  int row = pos / SCREEN_WIDTH; // 光标在屏幕的第row行
  int col = pos % SCREEN_WIDTH; // 光标在屏幕的第col行

  // 从屏幕第0行开始，找到第row行
  line* tmp = tx.show;
  while(row--){
    tmp = tmp->next;
  }

  uchar c;
  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    // 在第row行的col列插入字符c
    insertc(tmp, col, c);
    // 重新打印该行以及之后的行（打印前光标先移动到行首）
    setcurpos(row * SCREEN_WIDTH,0);
    printlines(row, tmp);
    // 光标设置到第row行的col+1列的位置，显示该位置的字符
    setcurpos(pos+1, tmp->chs[col+1]);
    pos++;
    row = pos / SCREEN_WIDTH; // 光标在屏幕的第row行
    col = pos % SCREEN_WIDTH; // 光标在屏幕的第col行
  }
}

// 主程序
void
editor(void)
{
  int editflag = 0;
  uchar c;
  printlines(0, tx.show);
  // 光标移至左上角（pos=0），并输出该位置的字符
  setcurpos(0, getcatpos(0));
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
      insert();
      // TODO: 进入编辑模式
      //printf(1,"i!");
      break;

    // 方向键上
    case KEY_UP:
      // TODO
      printf(1,"UP");
      break;
    // 方向键下
    case KEY_DN:
      // TODO
      printf(1,"DN");
      break;
    // 方向键左
    case KEY_LF:
      // TODO
      printf(1,"LF");
      break;
    // 方向键右
    case KEY_RT:
      // TODO
      printf(1,"RT");
      break;

    // ESC
    case KEY_ESC:
      // TODO
      printf(1,"ESC");
      break;

    // case 'e' 只是调试用的
    case 'e':
      return;

    // TODO: 添加其他case
    default:
      break;
    }
  }

    if(editflag)
      wirtetopath();
  }

// 从指定的文件路径中读取所有内容，并组织成行结构（双向链表），出错时返回-1
int
readtext(char *path)
{
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  uint nbytes;            // 文件大小（字节数）
  char *chs;              // 文件中的所有字符

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
    chs = (char*) malloc(nbytes);
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
  tx.head = tx.tail = tx.show = newlines(chs, nbytes);

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