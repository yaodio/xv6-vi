#define NULL          0       // 空指针

#define MAX_COL SCREEN_WIDTH  // 每行最大字符数设为屏幕宽度
// 行结构体（双向链表节点）
typedef struct line {
  ushort chs[MAX_COL];        // 一行字符(每个字符2字节，高8位是颜色，低8位是ascii码)
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
  line *show;                 // 屏幕上显示的第0行
} tx = {NULL,NULL,NULL,NULL}; // 全局文档变量

// 键盘按键
#define KEY_UP        0xE2    // 上
#define KEY_DN        0xE3    // 下
#define KEY_LF        0xE4    // 左
#define KEY_RT        0xE5    // 右
#define KEY_ESC       0x1B    // ESC