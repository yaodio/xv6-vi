#ifndef VI_XV6_TEXT_H
#define VI_XV6_TEXT_H

// 文本结构体（双向链表，每个节点表示一行）
typedef struct text {
    char *path;                 // 文件路径
    line *head;                 // 首行
    int exist;                  // 为0时表示路径指向的文件不存在
    int nchar;                  // 字符数
} text;

int readtext(char *path, text *tx);
int writetext(text *tx);
void freetx(text *tx);
#endif // VI_XV6_TEXT_H