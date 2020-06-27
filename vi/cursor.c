#include "vi.h"

// 光标下移, 无法移动则返回0
int
curdown(cursor *cur)
{
  // 已经是文档最后一行，无法下移
  if(cur->l->next == NULL){
    // 光标已经在最后一行尾部，无需操作
    if(cur->col == cur->l->n)
      return 0;
    // 否则移到行尾
    cur->col = cur->l->n;
  }
  else{
    // 特殊情况时光标指向当前行行尾，但显示位置在下一行行首
    // 那么就修改指针指向下一行行首
    if(cur->col == MAX_COL){
      cur->col = 0;
      cur->row++;
      cur->l = cur->l->next;
    }
      // 光标在当前行中（首）
    else{
      cur->row++;
      cur->l = cur->l->next;
      // 下一行的这个位置没有字符，则左移到最后一个字符的位置
      if(cur->col > cur->l->n)
        cur->col = cur->l->n;
    }

    // 光标移到了底线行，需要整个屏幕内容下移一行打印
    if(cur->row >= BASE_ROW){
      printlines(0, getprevline(cur->l, cur->row)->next, 1);
      cur->row = BASE_ROW - 1;
    }
  }
  showcur(cur);
  return 1;
}

// 光标上移, 无法移动则返回0
int
curup(cursor *cur)
{
  // 已经是文档首行，无法上移
  if(cur->l->prev == NULL){
    // 光标已经在首部，无需操作
    if(cur->col == 0)
      return 0;
    // 否则移到行首
    cur->col = 0;
  }
  else{
    // 特殊情况时光标指向在当前行行尾（MAX_COL处），但显示位置在下一行行首
    // 那么移到当前行行首
    if(cur->col == MAX_COL)
      cur->col = 0;
      // 光标在行中（首）
    else{
      cur->row--;
      cur->l = cur->l->prev;
      // 上一行的这个位置没有字符，则左移到最后一个字符的位置
      if(cur->col > cur->l->n)
        cur->col = cur->l->n;
    }

    // 需要屏幕上移一行打印
    if(cur->row < 0){
      cur->row = 0;
      printlines(0, cur->l, 1);
    }
  }
  showcur(cur);
  return 1;
}

// 光标左移, 无法移动则返回0
int
curleft(cursor *cur)
{
  cur->col--;

  // 底线模式第0列为':'
  if(cur->row == BASE_ROW && cur->col < 1) {
    cur->col = 1;
    return 0;
  }

  // 在文档首行
  if(cur->l->prev == NULL){
    // 已经开头，无法左移
    if(cur->col < 0){
      cur->col++;
      return 0;
    }
  }
    // 不是文档首行
  else{
    // 在开头左边，则移到上一行尾部
    if(cur->col < 0){
      cur->row--;
      cur->l = cur->l->prev;
      cur->col = cur->l->n;
      // 上一行是同一段（因此也一定是满的），cur.col移到最后一个字符处（n-1, 同时也是 MAX_COL-1）
      // 否则如果上一行是满的但不同一段，cur.col留在MAX_COL位置（特殊情况）
      if(cur->l->paragraph)
        cur->col--;
    }
  }

  // 需要屏幕上移一行打印
  if(cur->row < 0){
    cur->row = 0;
    printlines(0, cur->l, 1);
  }
  showcur(cur);
  return 1;
}

// 光标右移, 无法移动则返回0
int
curright(cursor *cur)
{
  int n = cur->l->n;

  cur->col++;

  // 底线模式
  if(cur->row == BASE_ROW){
    if(cur->col > n || cur->col == MAX_COL){
      cur->col--;
      return 0;
    }
    showcur(cur);
    return 1;
  }

  // 在文档最后一行
  if(cur->l->next == NULL){
    // 已经超过尾部，无法右移
    if(cur->col > n){
      cur->col = n;
      return 0;
    }
  }
    // 不是文档的最后一行
  else{
    // 当前行未满（下一行必然不同段）
    if(n < MAX_COL){
      // 可以指向下一行
      if(cur->col > n){
        cur->row++;
        cur->col = 0;
        cur->l = cur->l->next;
      }
    }
      // 当前行已满
    else{
      // col >= n 看下一行是否同段
      if(cur->col >= n){
        // 同段 移到下一行
        if(cur->l->paragraph){
          cur->row++;
          cur->col -= n;
          cur->l = cur->l->next;
        }
          // 不同段 且 col>n 移到下一行（col=n则不需要改变，是特殊情况）
        else if(cur->col > n){
          cur->row++;
          cur->col = 0;
          cur->l = cur->l->next;
        }
      }
    }
  }

  // 光标移到了底线行，需要整个屏幕内容下移一行打印
  if(cur->row >= BASE_ROW){
    printlines(0, getprevline(cur->l, cur->row)->next, 1);
    cur->row = BASE_ROW - 1;
  }
  showcur(cur);
  return 1;
}

// 将光标设置到屏幕上, 确保row在 [0, BASE_ROW]，col在 [0, MAX_COL] 范围内
void
showcur(cursor *cur)
{
  int pos;
  // 计算光标的位置
  pos = MAX_COL * cur->row + cur->col;

  // 光标所在行为倒数第二行（底线行的上一行）的末尾（光标所在列为MAX_COL）
  // 需要整个屏幕内容下移一行打印
  if(cur->col == MAX_COL && cur->row == BASE_ROW-1){
    printlines(0, getprevline(cur->l, cur->row)->next, 1);
    pos -= MAX_COL;
    cur->row--;
  }

  setcurpos(pos);
}

// 移动光标至某处
void
curto(cursor *cur, int row, int col, line* l) {
  if(col < 0 || col > MAX_COL){
    // TODO: error cur pos
  }else if(row < 0 || row > BASE_ROW){
    // TODO: error cur pos
  }else if(row == BASE_ROW && col == MAX_COL){
    // TODO: error cur pos
  }else{
    cur->row = row; cur->col = col; cur->l = l;
    showcur(cur);
  }
}
