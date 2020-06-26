#ifndef VI_XV6_CURSOR_H
#define VI_XV6_CURSOR_H

// 光标结构体
typedef struct cursor {
    int row;                    // 光标所在行
    int col;                    // 光标所在列
    line *l;                    // 光标指向的行节点
} cursor;

// 函数声明
int curdown(cursor *cur);
int curup(cursor *cur);
int curleft(cursor *cur);
int curright(cursor *cur);

void showcur(cursor *cur);
void curto(cursor *cur, int row, int col, line* l);

#endif // VI_XV6_CURSOR_H