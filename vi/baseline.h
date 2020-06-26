#ifndef VI_XV6_BASELINE_H
#define VI_XV6_BASELINE_H

#define MAX_PATH_CHAR   35
#define COOR_IDX        60

#define MSG_COLOR       getcolor(WHITE, DARK_GREY)
#define CMD_COLOR       getcolor(YELLOW, DARK_GREY)
#define ERROR_COLOR     getcolor(YELLOW, RED)

// 底线模式部分
enum { NOCHANGE, SAVE, QUIT, ERROR };

int baselinehandler(line*, int);
int savefile(line*);
void showpathmsg(void);
void showinsertmsg(void);
int baselinemode(int edit);

#endif //VI_XV6_BASELINE_H
