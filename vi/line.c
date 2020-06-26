#include "vi.h"

extern cursor cur;

// 在屏幕上第row行打印指定的行
void
printline(int row, line *l, int refreshcolor)
{
  int pos, i;

  if (refreshcolor)
    beautify();

  pos = row * SCREEN_WIDTH;
  for(i = 0; i < MAX_COL; i++)
    putcc(pos+i, paintc(l->chs[i], l->colors[i]));
}

// 从屏幕上第 row 行开始打印指定行 l 以及其后面的行，直到屏幕写满到BASE_ROW-1行
void
printlines(int row, line *l, int refreshcolor)
{
  line *blank = newlines(NULL, 0);
  
  if (refreshcolor)
    beautify();

  while(row < BASE_ROW){
    if(l != NULL){
      printline(row++, l, 0);
      l = l->next;
    }else{
      printline(row++, blank, 0);
    }
  }
}

// 设置指定行的字符、颜色，确保字符数n <= MAX_COL
void
setline(line *l, uchar *chs, int n, uchar color)
{
  memset(l->chs, '\0', MAX_COL);
  memset(l->colors, color, MAX_COL);
  memmove(l->chs, chs, n);
  l->n = n;
}

// 创建新的空白行
line*
newblankline(void)
{
  line *l = (line*)malloc(sizeof(line));
  setline(l, NULL, 0, DEFAULT_COLOR);
  l->paragraph = 0;
  l->prev = l->next = NULL;
}

// 根据传入的字符数组，构造双向链表，每个节点是一行
line*
newlines(uchar *chs, uint n)
{
  int i;
  line *l = newblankline();

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

// 释放多行字符的内存空间
void
freelines(line *l)
{
  line *next;

  while(l){
    next = l->next;
    free(l);
    l = next;
  }
}

// 返回指定行往前推的第i个前驱，如果前驱少于i个，则返回最前面的那个前驱
line*
getprevline(line *l, int i)
{
  while(i-- > 0 && l->prev != NULL)
    l = l->prev;
  return l;
}

// 在指定行的第i个字符处换行
void
breakline(line *l, int i)
{
  int j;
  line *newl;

  // 当前行的下一行属于同一段
  if(l->paragraph){
    // 可能与上一段同行，去掉段落标记即可
    if(i == 0 && l->prev != NULL && l->prev->paragraph)
      l->prev->paragraph = 0;
    else{
      // 将i以及后面的所有字符插入到下一行行首
      for(j = l->n-1; j>=i; j--){
        insertc(l->next, 0, l->chs[j]);
        l->chs[j] = '\0';
        l->n--;
      }
      l->paragraph = 0;
      cur.row++;
      cur.col = 0;
      cur.l = l->next;
    }
  }
    //如果下一行属于另一段，则将i以及后面的所有字符插入到新一行.
  else{
    newl = newlines(l->chs+i, l->n-i);
    memset(l->chs+i, '\0', l->n-i);
    l->n = i;
    if(l->next != NULL){
      l->next->prev = newl;
      newl->next = l->next;
    }
    newl->prev = l;
    l->next = newl;

    cur.row++;
    cur.col = 0;
    cur.l = l->next;
  }

  // 光标移到了底线行，需要整个屏幕内容下移一行打印
  if(cur.row >= BASE_ROW){
    printlines(0, getprevline(cur.l, cur.row)->next, 1);
    cur.row = BASE_ROW - 1;
  }else
    printlines(cur.row-1, cur.l->prev, 1);
  showcur(&cur);
}
