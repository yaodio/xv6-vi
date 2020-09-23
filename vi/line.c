#include "vi.h"

extern cursor cur;

// 在屏幕上第row行打印指定的行
void
printline(int row, line *l, int refreshcolor)
{
  int pos, i;

  if (refreshcolor)
    beautify();

  pos = row * MAX_COL;
  for(i = 0; i < MAX_COL; i++)
    putcc(pos+i, paintc(l->chs[i], l->colors[i]));
}

// 从屏幕上第 row 行开始打印指定行 l 以及其后面的行，直到屏幕写满到BASE_ROW-1行
void
printlines(int row, line *l, int refreshcolor)
{
  line *blank = newblankline();
  
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

  free(blank);
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
  return l;
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

  logging((TRACE_FILE, "[newlines] "), (TRACE_FILE, "creating new lines\n"));
  for(i = 0; i < n; i++){
    logging((TRACE_FILE, "[newlines] "), (TRACE_FILE, "creating new line %d/%d\n", i + 1, n));
    // 换行
    if(chs[i] == '\n'){
      l->next = newlines(chs+i+1, n-i-1); // FIXME: remove recursion, other wise it will trap
      l->next->prev = l;
      break;
    }
    // 写满一行，则下一行与该行同段
    if(i >= MAX_COL){
      l->next = newlines(chs+i, n-i); // FIXME: remove recursion, other wise it will trap
      l->next->prev = l;
      l->paragraph = 1;
      break;
    }
    // 填写一行中
    l->chs[i] = chs[i];
    l->n++;
  }

  logging((TRACE_FILE, "[newlines] "), (TRACE_FILE, "new line: %s\n", l->chs));
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

// 返回指定行往后推的第i个后继，如果后继少于i个，则返回最后面的那个后继
line*
getnextline(line *l, int i)
{
  while(i-- > 0 && l->next != NULL)
    l = l->next;
  return l;
}
