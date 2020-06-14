#include "../types.h"
#include "../user.h"

#include "vi.h"
#include "color.h"
#include "re.h"

extern struct text tx;

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

int//匹配两个字符串
matching(uchar str1[],int start_pos1,uchar str2[],int start_pos2,int cpnum)
{
  int i;
  for(i=0;i<cpnum;i++)
    if(str1[start_pos1+i] != str2[start_pos2+i])
      return  0;
  return 1;
}
void
beautify_cfile()
{
  uchar c_wordslib[15][10] = {"main",
                              "printf",
                              "exit",
                              "#include",
                              "for",
                              "if",
                              "break",
                              "int",
                              "char",
                              "float",
                              "double"
                              "=",
                              ">",
                              "<"
                              "*"};
  uchar c_colorlib[15] = {(BLACK << 4) | GREEN,
                          (BLACK << 4) | GREEN,
                          (BLACK << 4) | GREEN,
                          (BLACK << 4) | RED,
                          (BLACK << 4) | RED,
                          (BLACK << 4) | RED,
                          (BLACK << 4) | RED,
                          (BLACK << 4) | YELLOW,
                          (BLACK << 4) | YELLOW,
                          (BLACK << 4) | YELLOW,
                          (BLACK << 4) | YELLOW,
                          (BLACK << 4) | LIGHT_CYAN,
                          (BLACK << 4) | LIGHT_CYAN,
                          (BLACK << 4) | LIGHT_CYAN,
                          (BLACK << 4) | LIGHT_CYAN};
  int c_lenthlib[15] = {4,6,4,8,3,2,5,3,4,5,6,1,1,1,1};
  int words_num = 15;
  line *temp = tx.head;
  while(1)
  {
    int i;
    for(i=0;i<temp->n;i++)
    {
      if(i==0)//查询词库并着色
      {
        int k;
        for(k=0;k<words_num;k++)
        {
          if(matching(temp->chs,0,c_wordslib[k],0,c_lenthlib[k]))
          {
            int j;
            for(j=0;j<c_lenthlib[k];j++)
              temp->colors[j] = c_colorlib[k];
            i+=c_lenthlib[k];
          }
        }
      }
      else if(temp->chs[i] == ' '||temp->chs[i] == '('||temp->chs[i] == ',')//查询词库并着色
      {
        int k;
        for(k=0;k<words_num;k++)
        {
          if(matching(temp->chs,i+1,c_wordslib[k],0,c_lenthlib[k]))
          {
            int j;
            for(j=0;j<c_lenthlib[k];j++)
              temp->colors[i+1+j] = c_colorlib[k];
            i+=c_lenthlib[k]+1;
          }
        }
      } //在词库中添加了运算符这种情况，但是有时候没有效果，所以暂时把运算符单独拿出来处理
      else if(temp->chs[i] == '*'||temp->chs[i] == '+'||temp->chs[i] == '-'||temp->chs[i] == '='||temp->chs[i] == '/')
        temp->colors[i] = (BLACK << 4) | LIGHT_CYAN;
      else
        temp->colors[i] = (BLACK << 4) | WHITE;//把其他字符变为白色
    }
    if(temp == tx.tail)
      break;
    temp = temp->next;
  }
}
// 根据文件类型来着色
void
beautify(void)
{
  int i,temp;
  for(i=0;i<sizeof(tx.path);i++)
    if(tx.path[i] == '.')
    {
      temp = i;
      break;
    }
  if(matching(tx.path, temp,".c",0,2)) //如果是C文件
    beautify_cfile();
}