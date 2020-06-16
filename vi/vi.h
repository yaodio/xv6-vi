#ifndef VI_XV6_VI_H
#define VI_XV6_VI_H

#include "../types.h"
#include "../console.h"

#define NULL          0                   // 空指针
#define MAX_COL       SCREEN_WIDTH        // 每行最大字符数设为屏幕宽度
#define BASE_ROW      (SCREEN_HEIGHT-1)   // 底线行的行号
#define TAB_WIDTH     4                   // \t的宽度

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
typedef struct text {
    char *path;                 // 文件路径
    line *head;                 // 首行
    line *tail;                 // 尾行
    int exist;                  // 为0时表示路径指向的文件不存在
    int word_count;
} text;

// 函数声明
void printline(int row, line *l);
void printlines(int row, line *l);
line* getprevline(line *l, int i);

#endif // VI_XV6_VI_H