#include "vi.h"
#include "cursor.h"
#include "baseline.h"
#include "vulib.h"

#include "../types.h"
#include "../user.h"

extern text tx;
extern cursor cur;

int
baselinehandler(line* baseline, int edit)
{
  if (strcmp(baseline->chs, ":q") == 0) {
    if(edit){
      memset(baseline->chs, 0, MAX_COL);
      memmove(baseline->chs, "Edited but not save (input q! to quit compulsorily)", 51);
      paintl(baseline, ERROR_COLOR);
      return ERROR;
    }
    return QUIT;
  } else if (strcmp(baseline->chs, ":q!") == 0){
    return QUIT;
  } else if (strcmp(baseline->chs, ":w") == 0){
    savefile();
    return SAVE;
  } else if (strcmp(baseline->chs, ":wq") == 0){
    savefile();
    return QUIT;
  }
  // TODO: 添加其他命令
  else {
    // ERROR
    memset(baseline->chs, 0, MAX_COL);
    memmove(baseline->chs, "Invalid command", 15);
    paintl(baseline, ERROR_COLOR);
    return ERROR;
  }
}

void
savefile(void)
{
  if (!(tx.path)) {
    // TODO: 输入保存路径
//    printf(2, "tx in baseline: %d, no path\n", &tx);
  }
  else writetext(tx.path, tx.head);
}

int
int2char(char* res, int xx)
{
  static char digits[] = "0123456789";
  char buf[16];
  int i, j, neg;
  uint x;

  neg = 0;
  if(xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    buf[i++] = digits[x % 10];
  }while((x /= 10) != 0);
  if(neg)
    buf[i++] = '-';

  j = 0;
  while(--i >= 0)
    res[j++] = buf[i];

  return j;
}

void
showpathmsg(void)
{
  int i = 0;
  int len = 0;
  int pos = SCREEN_WIDTH * BASE_ROW;
  uchar base[MAX_COL];
  char* filename;
  
  memset(base, '\0', MAX_COL);

  if((filename = getfilename(tx.path)) != NULL){
    base[i++] = '"';
    if((len = strlen(filename)) > MAX_PATH_CHAR){
      base[i++] = '.';
      base[i++] = '.';
      memmove(base+i, filename+(len-MAX_PATH_CHAR), MAX_PATH_CHAR);
      i+= MAX_PATH_CHAR;
    }else{
      memmove(base+i, filename, len);
      i+=len;
    }
    base[i++] = '"';
    base[i++] = ' ';
  }

  if(tx.exist == 0)
    memmove(base+i, "[New File]", 10);

  i = COOR_IDX;
  i += int2char(base+i, cur.row);
  base[i++] = ',';
  base[i++] = ' ';
  int2char(base+i, cur.col);

  for(i = 0; i < MAX_COL; i++)
    putcc(pos+i, paintc(base[i], MSG_COLOR));

  if(filename)
    free(filename);
}

void
showinsertmsg(void)
{
  int i;
  uchar base[MAX_COL];
  int pos = SCREEN_WIDTH * BASE_ROW;

  memset(base, '\0', MAX_COL);
  memmove(base, "-- INSERT --", 12);
  
  i = COOR_IDX;
  i += int2char(base+i, cur.row);
  base[i++] = ',';
  base[i++] = ' ';
  int2char(base+i, cur.col);

  for(i = 0; i < MAX_COL; i++)
    putcc(pos+i, paintc(base[i], MSG_COLOR));
}
