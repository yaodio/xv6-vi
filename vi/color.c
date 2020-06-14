#include "../types.h"
#include "../user.h"

#include "vi.h"
#include "color.h"

// 给字符c涂上颜色color（color已组合了文本色和背景色信息）
ushort
paintc(uchar c, uchar color)
{
  return (color << 8) | c;
}

// 给字符c涂上文本色tcolor和背景色bcolor
uchar
paintc_tb(uchar c, uchar tcolor, uchar bcolor)
{
  return (bcolor << 12) | (tcolor << 8) | c;
}

uchar getcolor(uchar tcolor, uchar bcolor)
{
  return (bcolor << 4) | tcolor;
}

void
paintl (line* l, uchar tcolor, uchar bcolor)
{
  for (int i = 0; i < MAX_COL; i++) l->colors[i] = getcolor(tcolor, bcolor);
}