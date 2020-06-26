#include "vi.h"
#include "cursor.h"
#include "baseline.h"
#include "vulib.h"

#include "../types.h"
#include "../user.h"

extern text tx;
extern cursor cur;

int
startswidth(uchar *chs, uchar *cmd, int n)
{
  int i;
  for(i = 0; i < n; i++)
    if(chs[i] != cmd[i])
      return 0;
  return 1;
}

int
baselinehandler(line* baseline, int edit)
{
  if(startswidth(baseline->chs, ":q!", 3)) {
    return QUIT;
  } else if(startswidth(baseline->chs, ":q", 2)){
    if(edit){
      setline(baseline, "Edited but not save (input q! to quit compulsorily)", 51, ERROR_COLOR);
      return ERROR;
    }
    return QUIT;
  } else if (startswidth(baseline->chs, ":wq", 3)){
    return savefile(baseline) ? QUIT : ERROR;
  } else if (startswidth(baseline->chs, ":w", 2)){
    return savefile(baseline) ? SAVE : ERROR;
  } else if (startswidth(baseline->chs, ":h", 2)) {
    cls();
    help_mode();
    return NOCHANGE;
  }
  // TODO: 添加其他命令
  else {
    // ERROR
    setline(baseline, "Invalid command", 15, ERROR_COLOR);
    return ERROR;
  }
}

int
savefile(line* baseline)
{
  char path[MAX_COL];
  int i;

  memset(path, '\0', MAX_COL);
  for(i = 0; i < MAX_COL && baseline->chs[i] != '\0'; i++)
    if(baseline->chs[i] == ' '){
      i++;
      break;
    }
  memmove(path, baseline->chs+i, baseline->n - i);
  if(strlen(path) > 0){
    strcpy(tx.path, path);
  }

  if(writetext(tx.path, tx.head) < 0){
    setline(baseline, "Failed to save", 14, ERROR_COLOR);
    return 0;
  }

  tx.exist = 1;
  return 1;
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
