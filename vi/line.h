#ifndef VI_XV6_LINE_H
#define VI_XV6_LINE_H

// 行结构体（双向链表节点）
typedef struct line {
    uchar chs[MAX_COL];         // 一行字符（每个字符1字节，ascii码）
    uchar colors[MAX_COL];      // 字符的颜色（每个颜色1字节，高4位是背景色，低4位文本色）
    uint n;                     // 该行的字符数，理论上应该 n <= SCREEN_WIDTH
    struct line *prev;          // 上一行指针
    struct line *next;          // 下一行指针
    int paragraph;              // 当为1时表示与下一行是同一段
} line;

void printline(int row, line *l, int refreshcolor);
void printlines(int row, line *l, int refreshcolor);
void setline(line *l, uchar *chs, int n, uchar color);
line* newblankline(void);
line* newlines(uchar *chs, uint n);
void freelines(line *l);
line* getprevline(line *l, int i);
line* getnextline(line *l, int i);

#endif // VI_XV6_LINE_H