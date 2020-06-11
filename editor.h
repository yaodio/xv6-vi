#define NULL          0       // 空指针

#define MAX_COL SCREEN_WIDTH  // 每行最大字符数设为屏幕宽度

// 行结构体（双向链表节点）
typedef struct line {
  uchar chs[MAX_COL];         // 一行字符（每个字符1字节，ascii码）
  uchar colors[MAX_COL];      // 字符的颜色（每个颜色1字节，高4位是背景色，低4位文本色）
  uint n;                     // 该行的字符数，理论上应该 n <= SCREEN_WIDTH
  struct line *prev;          // 上一行指针
  struct line *next;          // 下一行指针
  int paragraph;              // 当为1时表示与下一行是同一段
} line;

// 文本结构体（双向链表，每个节点表示一行）
static struct text {
  char *path;                 // 文件路径
  line *head;                 // 首行
  line *tail;                 // 尾行
} tx = {NULL,NULL,NULL,NULL}; // 全局文档变量

// 光标结构体
static struct cursor {
  int row;                    // 光标所在行
  int col;                    // 光标所在列
  line *l;                    // 光标指向的行节点
} cur = {0, 0, NULL};         // 全局光标变量

// 底线模式部分
enum { NORMAL, QUIT };
int basehead;               // 底线首行
#define CMD_MODE        1

int mode = 0;