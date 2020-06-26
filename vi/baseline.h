#ifndef VI_XV6_BASELINE_H
#define VI_XV6_BASELINE_H

#define MAX_PATH_CHAR   35      // 底线显示路径的最多字符数
#define COOR_IDX        60      // 底线显示光标坐标信息的位置

#define MSG_COLOR       getcolor(WHITE, DARK_GREY)
#define CMD_COLOR       getcolor(YELLOW, DARK_GREY)
#define ERROR_COLOR     getcolor(YELLOW, RED)

// 底线模式返回的状态
enum { NOCHANGE, HELP, SAVE, QUIT, ERROR };

void showpathmsg(void);
void showinsertmsg(void);
int baselinemode(int edit);

#endif //VI_XV6_BASELINE_H
