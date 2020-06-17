#include "../types.h"
#include "../user.h"

#include "vi.h"
#include "color.h"
#include "re.h"
#include "stl.h"
#include "../stat.h"
#include "../fcntl.h"

extern struct text tx;
map_t regex_map, colormap;
list* syntax_keys;

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

// 返回下一个单词
char*
sscanf (char* chs)
{
  int i = 0;
  while (*(chs + i) != '\0' && *(chs + i) != ' '
      && *(chs + i) != '\t' && *(chs + i) != '\n')
    i++;
  if (*(chs + i) == '\n') return 0;
  char *res = malloc((i + 1) * sizeof(char));
  memmove(res, chs, i);
  chs += i;
  // 跳过空格
  while (*chs != '\0' &&
      (*chs == ' ' || *chs == '\t'))
    chs++;
  return res;
}

void
read_syntax ()
{
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
  regex_map = hashmap_new();
  colormap = hashmap_new();
  syntax_keys = new_list();

  int i;
  for (i = strlen(tx.path)-1; i >= 0; i--)
    if (tx.path[i] == '.') break;
  char *vi_file = malloc((strlen(tx.path) - i + 3) * sizeof(char));
  memmove(vi_file, tx.path + i + 1, strlen(tx.path) - i - 1);
  memmove(vi_file + i, ".vi\0", 4);

  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  if ((fd = open(vi_file, O_RDONLY)) >= 0) {
    if (fstat(fd, &st) < 0) {
      printf(2, "no syntax file %s\n", vi_file);
      close(fd);
      return;
    }
    if(st.type != T_FILE){
      printf(2, "editor: cannot edit a directory: %s\n", vi_file);
      close(fd); // 与open匹配
      return;
    }

    uchar *chs = (uchar *) malloc(st.size);
    read(fd, chs, st.size);
//    printf(1, "open file succeed\n%s", chs);
    int error;
    while (*chs != '\0') {
      char* type = sscanf(chs); // 类型，keyword 或 hi
      if (strcmp(type, "keyword") == 0) {
        // keyword 关键字
        char* key = sscanf(chs);
        if (*chs == '\0') return;
        push_back(syntax_keys, (int) key);
        char* regex;
        while ((regex = sscanf(chs)) != 0) {
          line* lst = new_list();
          error = hashmap_get(regex_map, key, (void**)(&lst));
          push_back(lst, (int) regex);
          if (error != MAP_OK)
            hashmap_put(regex_map, key, lst);
          if (*chs == '\0') return;
        }
        chs++; // 跳过换行
      } else if (strcmp(type, "hi") == 0) {
        // hi 高亮颜色
        char* key = sscanf(chs);
        if (*chs == '\0') return;
        char* color = sscanf(chs);
        hashmap_put(colormap, key, color);
        chs++;
      } else return;
    }
  } else printf(2, "cannot open file %s\n", vi_file);
  close(fd); // 与open匹配
  free(vi_file);
}

// 返回所有行拼接的字符串（不含换行）
char*
concat_file ()
{
  char* chs = malloc(tx.nchar * sizeof(char) + 1);
  int i = 0;
  for (line* l = tx.head; l != NULL; l = l->next) {
    memmove(chs + i, l->chs, l->n);
    i += l->n;
  }
  return chs;
}

// 根据文件类型来着色
void
beautify(void)
{
   /* // when beautify
   * 把 tx 里 line 合成一个 char 文本
   * 对每一个规则名字:
   *    pattern = re_compile(regex_map[规则名字] 里每一条正则)
   *    index_list = re_match_all(pattern, 文本, 长度 list)
   *    便利 index_list:
   *        找到 index 对应 line 中的字符
   *        把字符染成colormap[规则名字]
   * */
   char* chs = concat_file();
   uint* colors = malloc(strlen(chs) * sizeof(uint));
   // 遍历所有 keyword
   for (int_node* p = syntax_keys->head; p != NULL; p = p->next) {
     list* regex = new_list();
     hashmap_get(regex_map, (char*) p->data, regex);
     // 遍历 keyword 对应的所有正则表达式
     for (int_node* reg = regex->head; reg != NULL; reg = reg->next) {
       // 编译正则
       re_t pattern = re_compile((char *) reg->data);
       list *match_length = new_list();
       list *matches = re_match_all(pattern, chs, match_length);
       // 遍历所有匹配到的字符的 index
       for (int_node *r = matches->head; r != NULL; r = r->next) {
         // TODO: colors[q->data] = hashmap_get(keyword)
         // 爷写不动了
       }
     }
   }

   free(chs);
}