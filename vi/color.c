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
  char* chs = malloc(tx.word_count * sizeof(char) + 1);
  int i = 0;
  for (line* l = tx.head; l != NULL; l = l->next) {
    memmove(chs + i, l->chs, l->n);
    i += l->n;
  }
  return chs;
}

uint
find_color(char* cname)
{
  int l = sizeof(color_name) / sizeof(color_name[0]);
  for (int i = 0; i < l; i++)
    if (strcmp(cname, color_name[i]) == 0) return color_int[i];
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
   char* chs = concat_file(); // 把所有行的字符合成到一起
   uint* colors = malloc(strlen(chs) * sizeof(uint));
   int error; uint color_f; int i;
   char* syntax_color; // 存放 hashmap 中查到的颜色字符串
   int_node *idx, *jdx, *reg, *key;
   list *regex;
   // 遍历所有 keyword
   for (key = syntax_keys->head; key != NULL; key = key->next) {
     hashmap_get(regex_map, (char*) key->data, (void**)(&regex));
     // 遍历 keyword 对应的所有正则表达式
     for (reg = regex->head; reg != NULL; reg = reg->next) {
       // 编译正则
       re_t pattern = re_compile((char *) reg->data);
       list *match_length = new_list();
       list *matches = re_match_all(pattern, chs, match_length); // 匹配正则
       // 拿到 hashmap 中对应的颜色
       error = hashmap_get(colormap, (char*) key->data, (void**)(&syntax_color));
       if (error != MAP_OK) color_f = WHITE; // 找不到，默认白色
       // 是数字（十进制）就直接放进去，否则查颜色表
       else color_f = (syntax_color[0] >= '0' && syntax_color[0] <= '9') ?
           atoi(syntax_color) : find_color(syntax_color);
       // 遍历所有匹配到的字符的 index，改颜色
       for (idx = matches->head, jdx = match_length->head; (idx != NULL && jdx != NULL);
        idx = idx->next, jdx = jdx->next)
         memset(colors+idx->data, color_f, jdx->data * sizeof(colors[0])); // 把idx后面jdx长度的字符颜色都改一下
     }
   }
   // 将颜色信息同步到每行的 colors
   int i = 0, j;
   for (line* l = tx.head; l != NULL; l = l->next) {
     j = 0;
     while (l->chs[j] != '\0')
       l->colors[j++] = colos[i++];
   }

   free(chs);
   free(colors);
}