#include "vi.h"

extern uchar colorname[16][32];
extern uint colorint[16];

static char logoc[] = "              __          _\n"
               "__  ____   __/ /_   __   _(_)\n"
               "\\ \\/ /\\ \\ / / '_ \\  \\ \\ / / |\n"
               " >  <  \\ V /| (_) |  \\ V /| |\n"
               "/_/\\_\\  \\_/  \\___/    \\_/ |_|\n"
               "\n";

static char helpc[] = "https://gitee.com/yaodio/xv6   *STAR US!\n"
               "a simple editor for xv6\n"
               "with some commands similar to [vi]\n"
               "[i] insert before         \n"
               "[a] append after          \n"
               "[:] baseline mode         \n"
               "[:w] save file            \n"
               "[:q] quit                 \n"
               "[:q!] force quit          \n"
               "[:wq] save and quit       \n"
               "[:h] help (here)          \n"
               "[esc] quit baseline mode  \n"
               "\n"
               "[q (here)] quit help mode \n";

static text helptx = {NULL, NULL, 0, 0};

line*
makehelplines(void)
{
  line* l = newlines(logoc, strlen(logoc));
  uint len, margin;
  int i, j = 2, top = 0;
  int color_len = sizeof(colorname) / sizeof(colorname[0]);
  // 垂直居中
  line* p = NULL; char tmp[MAX_COL];
  for (p = l;; p = p->next, j+=1, top++) {
    len = p->n;
    if (len > 0) {
      margin = (MAX_COL - len) / 2;
      strcpy(tmp, p->chs);
      for (i = 0; i < len; i++)
        p->chs[margin + i] = tmp[i];
      for (i = 0; i < margin; i++)
        p->chs[i] = ' ';
      p->n = margin + len;
      paintl(p, colorint[j % color_len]); // 上渐变色
    }
    if (p->next == NULL) break;
  }
  // 添加 help text
  line* help_l = newlines(helpc, strlen(helpc));
  p->next = help_l; help_l->prev = p;
  // help_text 垂直居中
  for (p = help_l; p != NULL; p = p->next) {
    len = p->n;
    if (len > 0) {
      margin = (MAX_COL - len) / 2;
      strcpy(tmp, p->chs);
      for (i = 0; i < len; i++)
        p->chs[margin + i] = tmp[i];
      for (i = 0; i < margin; i++)
        p->chs[i] = ' ';
      p->n = margin + len;
    }
  }
  // logo 水平居中
  top = (SCREEN_HEIGHT - top) / 2 - 8;
  while (top--) {
    l->prev = newlines(NULL, 0);
    l->prev->next = l; l = l->prev;
  }

  return l;
}

void
inithelptx(void)
{
  helptx.head = makehelplines();
  helptx.exist = 1;
}

void
freehelptx(void)
{
  if(helptx.exist)
    freelines(helptx.head);
}

void
help(void)
{
  uchar c;

  setcurpos(0);
  if(!helptx.exist)
    inithelptx();
  
  cls();
  printlines(0, helptx.head, 0);
  while ((c = readc()) != 'q')
    ;
  cls();
}