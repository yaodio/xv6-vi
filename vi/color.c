#include "../types.h"
#include "../user.h"

#include "vi.h"
#include "color.h"
#include "re.h"
#include "stl.h"

extern struct text tx;

// 给字符c涂上颜色color（color已组合了文本色和背景色信息）
ushort
paintc(uchar c, uchar color)
{
  return (color << 8) | c;
}

// 组合文本色tcolor和背景色bcolor
uchar getcolor(uchar tcolor, uchar bcolor)
{
  return (bcolor << 4) | tcolor;
}

void
paintl(line* l, uchar color)
{
  memset(l->colors, color, MAX_COL);
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

void hashmap_test () {
  map_t mymap;
  mymap = hashmap_new();
  hashmap_put(mymap, "test", 127687);
  int value;
  hashmap_get(mymap, "test", (void*)(&value));
  printf(2, "hashmap test: <test, %d>", value);
  hashmap_free(mymap);
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
//提取出一行中的单词，返回一个二维字符数组，一行一个单词
char** 
fetch_words(uchar* file,int start_pos,int end_pos,int wordsnum)
{
  char **words = (char **)malloc(wordsnum*sizeof(char*));
  int i;
  int flag = 0;
  for(i=start_pos;i<=end_pos;i++)
  {
    if(i == start_pos || file[i-1] == ' ')
    {
      int j;
      int word_lenth = 0;
      for(j=i;j<=end_pos;j++)
      {
        if(file[i]==' ')
          break;
        word_lenth++;
      }
      words[i] = (char *)malloc(word_lenth*sizeof(char));
       for(j=0;j<=word_lenth;j++)
       {
         words[i][j] =file[i+j]; 
       }
    }
  }
  return words;
}
// 根据文件类型来着色
void
beautify(void)
{
//  int i,temp;
//  for(i=0;i<sizeof(tx.path);i++)
//    if(tx.path[i] == '.')
//    {
//      temp = i;
//      break;
//    }
//  if(matching(tx.path, temp,".c",0,2)) //如果是C文件
//    beautify_cfile();

  // 伪代码
  /**
   * // when 读取文件
   * read 文件后缀名.vi
   * 便利 后缀名.vi
   *    switch (开头那个词) {
   *    case "keyword":
   *        regex_map[规则名字].push_back(正则)
   *    case "hi":
   *        colormap[规则名字] = 颜色
   *    }*/
  map_t regex_map;
  regex_map = hashmap_new();
  map_t colormap;
  colormap = hashmap_new();

  char *vi_file = "xxx.vi";
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  int fd = open(vi_file, O_RDONLY);
  fstat(fd, &st);
  uchar* the_file = (uchar*)malloc(st.size);
  read(fd, the_file, st.size);
  close(fd); // 与open匹配
  for(int i = 0 ;i < st.size;i++)
  {
    if(i==0 || the_file[i-1] == '\n')
    {
      int end_pos = st.size;
      int words_num = 0;
      for(int j = i;j<st.size;j++)
      {
        if(the_file[j] == '\n'||the_file[j] == '\0')
        {
          end_pos = j-1;
          break;
        }
        else if(the_file[j] == ' ')
         words_num++;
      }
      words_num++;
      char ** words = fetch_words(the_file,i,end_pos,words_num);
      if(matching(words[0],0,"keywords",0,7))
      {
        for(int k = 2;k<words_num;k++)
        {
          hashmap_put(regex_map, words[1],words[k]);
        }
      }
      else if(matching(words[0],0,"hi",0,2))
      {
        hashmap_put(colormap, words[1],words[2]);
      }
    }
  }
   /* // when beautify
   * 把 tx 里 line 合成一个 char 文本
   * 对每一个规则名字:
   *    pattern = re_compile(regex_map[规则名字] 里每一条正则)
   *    index_list = re_match_all(pattern, 文本, 长度 list)
   *    便利 index_list:
   *        找到 index 对应 line 中的字符
   *        把字符染成colormap[规则名字]
   * */
}