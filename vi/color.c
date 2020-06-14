#include "../types.h"
#include "color.h"

// 给字符c涂上颜色color（color已组合了文本色和背景色信息）
ushort
paintc(uchar c, uchar color)
{
  return (color << 8) | c;
}

// 给字符c涂上文本色tcolor和背景色bcolor
ushort
paintc_tb(uchar c, uchar tcolor, uchar bcolor)
{
  return (bcolor << 12) | (tcolor << 8) | c;
}