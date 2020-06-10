#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "console.h"
#include "kbd.h"

#define NULL 0  // 空指针

// 行结构体（双向链表节点）
typedef struct line {
  char chs[SCREEN_WIDTH+1]; // 一行字符，+1是为了末尾填入'\0'
  uint n;                   // 该行的字符数，理论上应该 n <= SCREEN_WIDTH
  struct line *prev;        // 上一行指针
  struct line *next;        // 下一行指针
} line;

// 根据传入的字符数组，构造双向链表，每个节点是一行
line*
newlines(char *chs, uint n)
{
  int i;
  line *l = (line*)malloc(sizeof(line));
  l->n = 0;
  l->prev = l->next = NULL;
  
  // 空行
  if(chs == NULL || n == 0){
    l->chs[0] = '\0';
    return l;
  }

  for(i = 0; i < n; i++){
    // 换行
    if(chs[i] == '\n'){
      l->next = newlines(chs+i+1, n-i-1);
      l->next->prev = l;
      break;
    }
    // 写满一行
    if(i >= SCREEN_WIDTH){
      l->next = newlines(chs+i, n-i);
      l->next->prev = l;
      break;
    }
    // 填写一行中
    l->chs[i] = chs[i];
    l->n++;
  }
  l->chs[l->n] = '\0';
  
  return l;
}

// 文本结构体（双向链表，每个节点表示一行）
typedef struct text {
  char *path;   // 文件路径
  line *head;   // 首行
  line *tail;   // 尾行
  line *show;   // 屏幕上显示的第0行
} text;

// 打印所有行
void
printlines(text *tx)
{
  int nrow = 0;
  line *tmp = tx->show;

  while(tmp != NULL){
    // 保留底线行（最多输出 SCREEN_HEIGHT-1 行）
    if(nrow >= SCREEN_HEIGHT - 1)
      break;

    printf(1, "%s", tmp->chs);
    // 如果当前行不满80个字符，且还有下一行，才输出\n
    // 如果当前行不满80个字符，但没有下一行，不用输出\n
    // 若当前行已满80个字符，cga会自动换行（pos+1, 见console.c的cgaputc函数)，所以也不用显式输出\n
    if(tmp->next != NULL && tmp->n < SCREEN_WIDTH)
      printf(1, "\n");
    nrow++;
    tmp = tmp->next;
  }
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

// TODO：这个函数还没有测试过，等支持处理方向键后再测试
// 获取在光标pos位置上的字符
char
getcatpos(text *tx, int pos)
{
  int row, col;
  line *tmp;
  // TODO：检查pos的值，防止越界（可能第row行不存在，超出了双向链表范围）

  // pos = row * SCREEN_WIDTH + col
  row = pos / SCREEN_WIDTH; // 光标在屏幕的第row行
  col = pos % SCREEN_WIDTH; // 光标在屏幕的第col行

  // 从屏幕第0行开始，找到第row行
  tmp = tx->show;
  while(row--){
    tmp = tmp->next;
  }
  // 返回第row行的第col个字符
  return tmp->chs[col];
}

void
wirtetopath(text *tx)
{
  // TODO: 输出到tx->path，若路径不存在，则提示输入保存路径，或者直接退出不保存
}
void 
insert(text *tx)
{
  int pos = getcurpos();
  int row = pos / SCREEN_WIDTH; // 光标在屏幕的第row行
  int col = pos % SCREEN_WIDTH; // 光标在屏幕的第col行

  // 从屏幕第0行开始，找到第row行
  line* tmp = tx->show;
  while(row--){
    tmp = tmp->next;
  }
  // 返回第row行的第col个字符
  while(1){
    char c = readc();
    if(c=27) //Exit insert function
      break;
    int line_len = tmp->n;
    for(int i = line_len;i>col;i--)
      tmp->chs[i] = tmp->chs[i-1];
    tmp->chs[col] = c;
    col+=1;
    cls();
    printlines(tx);
    setcurpos(pos+1);
    pos++;
  }
}
void
editor(text *tx)
{
  int editflag = 0;
  char c;
  // TODO: 核心程序
  printlines(tx);
  setcurpos(0);
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
      insert(tx);
      // TODO: 进入编辑模式
      //printf(1,"i!");
      break;

    // 方向键上
    case KEY_UP:
      // TODO
      break;
    // 方向键下
    case KEY_DN:
      // TODO
      break;
    // 方向键左
    case KEY_LF:
      // TODO
      break;
    // 方向键右
    case KEY_RT:
      // TODO
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
      wirtetopath(tx);
  }

// 从指定的文件路径中读取所有内容，并组织成行结构（双向链表），并返回一个文本结构体指针text*
text*
readtext(char *path)
{
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  uint nbytes;            // 文件大小（字节数）
  char *chs;              // 文件中的所有字符
  text *tx;               // 文本结构体

  // 路径存在且可被打开
  if(path != NULL && (fd = open(path, O_RDONLY)) >= 0){
    // 获取文件信息失败则退出
    if(fstat(fd, &st) < 0){
      printf(2, "editor: cannot stat %s\n", path);
      close(fd); // 与open匹配
      return NULL;
    }

    // 否则检查文件信息，是否为文件（可能是目录）
    if(st.type != T_FILE){
      printf(2, "editor: cannot edit a directory: %s\n", path);
      close(fd); // 与open匹配
      return NULL;
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

  tx = (text*)malloc(sizeof(text));
  tx->path = path;
  // 将文件内容组织成行结构，并用指针进行标记
  tx->head = tx->tail = tx->show = newlines(chs, nbytes);

  // 定位尾行
  while(tx->tail->next != NULL){
    tx->tail = tx->tail->next;
  }

  return tx;  
}

// 命令行输入：editor [path]
// 文件路径path为可选参数，若无则在“临时文件”中使用editor
int
main(int argc, char *argv[])
{
  text *tx;               // 文件结构体
  ushort *backup;         // 屏幕字符备份
  int nbytes;             // 屏幕字符备份内容的字节大小

  // 读取文件，并组织成文本结构体
  tx = readtext(argc > 1 ? argv[1] : NULL);
  // 读取异常，退出
  if(tx == NULL){
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
  editor(tx);

  // 退出编辑器，开启控制台的flag，并还原屏幕上的所有字符
  consflag(1,1);
  rcs(backup, nbytes);
  free(backup);
  // TODO: free tx.
  exit();
}