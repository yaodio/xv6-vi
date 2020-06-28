#include "vi.h"

extern text tx;
extern cursor cur;

// 在指定行的第i个位置插入字符c，不用手动修改字符的颜色，beautify会处理
int
insertc(line *l, int i, uchar c)
{
  int j;
  uchar chs[1];
  line *newl;

  // 该行未满（此时i不可能为MAX_COL）
  if(l->n < MAX_COL){
    for(j = l->n; j > i; j--)
      l->chs[j] = l->chs[j-1];
    l->chs[i] = c;
    l->n++;
  }
    // 该行已满（此时i可能为MAX_COL）
  else{
    // i不为MAX_COL时，保存该行最后1个字符，放到下一行行首
    if(i < MAX_COL){
      chs[0] = l->chs[MAX_COL-1];
      for(j = MAX_COL-1; j > i; j--)
        l->chs[j] = l->chs[j-1];
      l->chs[i] = c;
    }
      // 否则c放到下一行行首
    else
      chs[0] = c;

    // 下一行是同一段
    if(l->paragraph)
      insertc(l->next, 0, chs[0]);
      // 下一行不是同一段，插入新行
    else{
      l->paragraph = 1;
      newl = newlines(chs, 1);
      newl->prev = l;
      newl->next = l->next;
      if(l->next)
        l->next->prev = newl;
      l->next = newl;
    }
  }

  return 1;
}

// 删除指定行的第i个字符
// 删除成功返回1, 失败返回0
int
deletec(line *l, int i)
{
  // 超过下标n没有字符可以删除（非法情况）
  if(i > l->n)
    return 0;
  // 此时 i <= l->n

  if(l->paragraph == 0){
    // 删除的是空行
    if(l->n == 0){
      // 文档最后一行且内容为空，不删除
      if(l->next == NULL)
        return 0;

      // 删除的是文档第一行（且不是最后一行）
      if(l->prev == NULL)
        tx.head = l->next;
      else
        l->prev->next = l->next;
      l->next->prev = l->prev;

      // 删除的是光标指向的行
      if(cur.l == l){
        cur.l = l->next;
        cur.col = 0;
      }
      free(l);
      return 1;
    }
    // 此时 i <= l->n 且 0 < l->n

    // 不涉及下一行的字符移动
    if(i < l->n){
      while(++i < l->n)
        l->chs[i-1] = l->chs[i];
      l->chs[i-1] = '\0';
      l->n--;
      // 没有字符了 且 跟上一行同一段
      if(l->n == 0 && l->prev != NULL && l->prev->paragraph == 1){
        l->prev->paragraph = 0;
        if(cur.l == l){
          cur.row--;
          cur.col = MAX_COL;
          cur.l = l->prev;
        }
        deletec(l, 0);
      }
      return 1;
    }
    // 此时 0 < i == l->n

    // 文档最后一个字符之后，无法删除
    if(l->next == NULL)
      return 0;

    if(l->n == MAX_COL){
      // 下一行是空行，删掉
      if(l->next->n == 0)
        deletec(l->next, 0);
        // 否则下一行变为同一段
      else
        l->paragraph = 1;

      if(cur.l == l && l->next != NULL){
        cur.l = l->next;
        cur.col = 0;
        cur.row++;
      }
      return 1;
    }
    // 此时 0 < i == l->n < MAX_COL(80), 即该行还有空位
    // 把下一行部分字符挪上来
    while(l->n < MAX_COL && l->next->n > 0){
      l->chs[l->n++] = l->next->chs[0];
      deletec(l->next, 0);
    }
    // 下一行被榨干了（变成空行），删掉
    if(l->next->n == 0)
      deletec(l->next, 0);
      // 否则下一行变为同一段
    else
      l->paragraph = 1;
  }
  else{
    // 同段时光标col（传入的i）不会等于MAX_COL
    // 此时 i < l->n == MAX_COL
    while(++i < l->n)
      l->chs[i-1] = l->chs[i];
    l->chs[i-1] = l->next->chs[0];
    deletec(l->next, 0);
  }

  return 1;
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

// 编辑模式，按ESC退出
int
editmode(void)
{
  int edit = 0;
  int tab;
  uchar c;

  showinsertmsg();
  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    switch(c){
      /**         方向键操作         **/
      case KEY_UP: curup(&cur);    break;  // 方向键上
      case KEY_DN: curdown(&cur);  break;  // 方向键下
      case KEY_LF: curleft(&cur);  break;  // 方向键左
      case KEY_RT: curright(&cur); break;  // 方向键右

      /**          删除操作          **/
      // 删除光标处前一个位置的字符
      case KEY_BACKSPACE:
        if(!curleft(&cur)) 
          break;
      // 删除光标处的字符
      case KEY_DEL:
        edit |= deletec(cur.l, cur.col);
        tx.nchar--;
        printlines(cur.row, cur.l, 1);
        break;

      /**          换行操作          **/
      case '\n':
        breakline(cur.l, cur.col);
        break;

      /**         制表符操作         **/
      case '\t':
        tab = TAB_WIDTH;
        while(tab--){
          // 在光标处插入字符c
          edit |= insertc(cur.l, cur.col, ' ');
          tx.nchar++;
        }
        // 重新打印该行（以及之后的行）
        if(cur.l->n == MAX_COL && cur.l->paragraph)
          printlines(cur.row, cur.l, 1);
        else
          printline(cur.row, cur.l, 1);

        tab = TAB_WIDTH;
        while(tab--)
          curright(&cur);
        break;

      /**        默认插入操作        **/
      default:
        // 在光标处插入字符c
        edit |= insertc(cur.l, cur.col, c);
        // 重新打印该行（以及之后的行）
        if(cur.l->n == MAX_COL && cur.l->paragraph)
          printlines(cur.row, cur.l, 1);
        else
          printline(cur.row, cur.l, 1);

        tx.nchar++;
        curright(&cur);
        break;
    }
  
  showinsertmsg();
  }

  return edit;
}
