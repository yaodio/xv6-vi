#include "../types.h"
#include "../user.h"

#include "vi.h"
#include "color.h"
#include "re.h"

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

void reg_test () {
  /* Standard int to hold length of match */
  int match_length;

  /* Standard null-terminated C-string to search: */
  const char* string_to_search = "ahem.. 'hello world !' ..";

  /* Compile a simple regular expression using character classes, meta-char and greedy + non-greedy quantifiers: */
  re_t pattern = re_compile("[Hh]ello [Ww]orld\\s*[!]?");

  /* Check if the regex matches the text: */
  int match_idx = re_matchp(pattern, string_to_search, &match_length);
  if (match_idx != -1)
  {
    printf(2, "match at idx %d, %d chars long.\n", match_idx, match_length);
  }
}