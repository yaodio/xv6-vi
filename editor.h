// 行结构体（双向链表节点）
typedef struct line {
  char chs[SCREEN_WIDTH+1]; // 一行字符，+1是为了末尾填入'\0'
  uint n;                   // 该行的字符数，理论上应该 n <= SCREEN_WIDTH
  struct line *prev;        // 上一行指针
  struct line *next;        // 下一行指针
} line;

// 文本结构体（双向链表，每个节点表示一行）
typedef struct text {
  char *path;   // 文件路径
  line *head;   // 首行
  line *tail;   // 尾行
  line *show;   // 屏幕上显示的第0行
} text;

// 键盘按键
#define KEY_UP        0xE2    // 上
#define KEY_DN        0xE3    // 下
#define KEY_LF        0xE4    // 左
#define KEY_RT        0xE5    // 右
#define KEY_ESC       0x1B    // ESC

#define NULL          0       // 空指针