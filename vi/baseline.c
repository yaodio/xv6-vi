#include "vi.h"

extern text tx;
extern cursor cur;
extern line baseline;

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

  if(writetext(&tx) < 0){
    setline(baseline, "Failed to save", 14, ERROR_COLOR);
    return 0;
  }

  tx.exist = 1;
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
    help();
    return HELP;
  }
  // TODO: 添加其他命令
  else {
    // ERROR
    setline(baseline, "Invalid command", 15, ERROR_COLOR);
    return ERROR;
  }
}

void
showpathmsg(void)
{
  int i = 0;
  int len = 0;
  int pos = MAX_COL * BASE_ROW;
  char base[MAX_COL];
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
  int pos = MAX_COL * BASE_ROW;

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

// 底线模式
int
baselinemode(int edit)
{
  int code = NOCHANGE;
  cursor oldcur = cur; // 保存光标
  setline(&baseline, ":", 1, CMD_COLOR);

  curto(&cur, BASE_ROW, 1, &baseline);
  printline(BASE_ROW, &baseline, 0);

  uchar c;
  // 循环读取1个字符，如果是ESC则结束
  while((c = readc()) != KEY_ESC){
    // 按下换行，处理命令
    if (c == '\n'){
      code = baselinehandler(&baseline, edit);
      break;
    }
    // 没有字符了且按下了退格
    if ((c == KEY_BACKSPACE && !curleft(&cur))) 
      break; 

    switch (c) {
      // 方向键左和上，光标左移
      case KEY_LF: case KEY_UP: curleft(&cur); break;
      // 方向键右和下，光标右移
      case KEY_RT: case KEY_DN: curright(&cur); break;
      // 删除光标处的字符
      case KEY_BACKSPACE: case KEY_DEL:
        deletec(cur.l, cur.col);
        printline(cur.row, cur.l, 0);
        break;
      // 插入字符
      default:
        insertc(cur.l, cur.col, c);
        printline(cur.row, cur.l, 0);
        curright(&cur);
    }
  }

  // 恢复光标
  cur = oldcur;
  showcur(&cur);

  return code;
}
