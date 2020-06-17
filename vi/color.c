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

uchar color_name[][32] = {"BLACK", "BLUE", "GREEN", "CYAN", "RED", "PURPLE", "BROWN", "GREY", "DARK_GREY", "LIGHT_BLUE",
                          "LIGHT_GREEN", "LIGHT_CYAN", "LIGHT_RED", "LIGHT_PURPLE", "YELLOW", "WHITE"};
uint color_int[] = {BLACK, BLUE, GREEN, CYAN, RED, PURPLE, BROWN, GREY, DARK_GREY, LIGHT_BLUE,
                    LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED, LIGHT_PURPLE, YELLOW, WHITE};

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
sscanf (uchar* chs, int* offset)
{
//  printf(1, "|%c...", *(chs+*offset));
  // 跳过空格和换行以免行开头有空格
  while (*(chs+*offset) != '\0' &&
         (*(chs+*offset) == ' ' || *(chs+*offset) == '\t'))
    *offset += 1;

  int i = 0;
  while (*(chs+*offset + i) != '\0' && *(chs+*offset + i) != ' '
      && *(chs+*offset + i) != '\t' && *(chs+*offset + i) != '\n')
    i++;
  if (*(chs+*offset) == '\n') i++; // 当offset指向换行时i将等于0，但我们希望把换行也返回回去
  char *res = malloc((i + 1) * sizeof(char));
  memmove(res, chs+*offset, i);
  res[i] = '\0';
//  printf(1, "%c|", *(chs+*offset + i-1));
  *offset += i;

  // 跳过空格，不会跳过换行
  while (*(chs+*offset) != '\0' &&
      (*(chs+*offset) == ' ' || *(chs+*offset) == '\t'))
    *offset += 1;
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
//  printf(2, "into read syntax\n");
  regex_map = hashmap_new();
  colormap = hashmap_new();
  syntax_keys = new_list();

  int i;
  for (i = strlen(tx.path)-1; i >= 0; i--)
    if (tx.path[i] == '.') break;
  if (i < 0) return;
  char *vi_file = malloc((strlen(tx.path) - i + 3) * sizeof(char));
  memmove(vi_file, tx.path + i + 1, strlen(tx.path) - i - 1);
  memmove(vi_file + i, ".vi\0", 4);
//  printf(2, "read %s\n", vi_file);

  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  uchar *chs = NULL; int offset = 0;
  if ((fd = open(vi_file, O_RDONLY)) >= 0) {
    if (fstat(fd, &st) < 0) {
      printf(2, "no syntax file %s\n", vi_file);
      close(fd);
      free(vi_file);
      return;
    }
    if(st.type != T_FILE){
      printf(2, "syntax: cannot read a directory: %s\n", vi_file);
      close(fd); // 与open匹配
      free(vi_file);
      return;
    }

    chs = (uchar *) malloc(st.size);
    read(fd, chs, st.size);
//    printf(1, "open file succeed\n%s|EOF", chs);
    int error;
    while (*(chs+offset) != '\0') {
      char* type = sscanf(chs, &offset); // 类型，keyword 或 hi
//      printf(1, "%s ", type);
      if (strcmp(type, "keyword") == 0) {
        char* key = sscanf(chs, &offset);
//        printf(1, "%s\n", key);
        // keyword 关键字
        if (*(chs+offset) == '\0') break;
        push_back(syntax_keys, (int) key);
        char* regex;
        for (regex = sscanf(chs, &offset);*regex != '\n'; regex = sscanf(chs, &offset)) {
//          printf(1, "%d %s, ", offset, regex);
          struct list* lst;
//          printf(1, "new list, ");
          error = hashmap_get(regex_map, key, (void**)(&lst));
          if (error != MAP_OK) lst  = new_list();
//          printf(1, "error: %d, ", error);
          push_back(lst, (int) regex);
//          printf(1, "push list, ");
          if (error != MAP_OK)
            hashmap_put(regex_map, key, lst);
//          printf(1, "put map\n");
          if (*(chs+offset) == '\0') break;
        }
//        printf(1, "\n");
      } else if (strcmp(type, "hi") == 0) {
        char* key = sscanf(chs, &offset);
//        printf(1, "%s\n", key);
        // hi 高亮颜色
        if (*(chs+offset) == '\0') break;
        char* color = sscanf(chs, &offset);
//        printf(1, "%s ", color);
        hashmap_put(colormap, key, color);
        offset++; // 跳过换行
//        printf(1, "\n");
      }
    }
  } else printf(2, "cannot open file %s\n", vi_file);
  close(fd); // 与open匹配
  free(vi_file);
  free(chs);
}

// 返回所有行拼接的字符串（不含换行）
char*
concat_file ()
{
  int line_num = 0;
  for (line* l = tx.head; l != NULL; l = l->next)
    if(!l->paragraph)
      line_num++;
  char* chs = malloc((tx.nchar + line_num) * sizeof(char));
  int i = 0, j;
  for (line* l = tx.head; l != NULL; l = l->next) {
//    printf(1, "%s  %d\n", l->chs, l->n);
    for (j = 0; j < (l->n); j++)
    {
      chs[i++] = l->chs[j];
    } 
    if(l->next!=NULL && !l->paragraph)
      chs[i++] = '\n';
  }
  return chs;
}

uint
find_color(char* cname)
{
  int l = sizeof(color_name) / sizeof(color_name[0]);
  for (int i = 0; i < l; i++) {
    if (strcmp(cname, color_name[i]) == 0) {
//      printf(1, "found %s: %d\n", color_name[i], color_int[i]);
      return color_int[i];
    }
  }
  return DEFAULT_COLOR;
}

// 根据文件类型来着色
void
beautify(void)
{
  if (syntax_keys->size == 0) return;
   /* // when beautify
   * 把 tx 里 line 合成一个 char 文本
   * 对每一个规则名字:
   *    pattern = re_compile(regex_map[规则名字] 里每一条正则)
   *    index_list = re_match_all(pattern, 文本, 长度 list)
   *    便利 index_list:
   *        找到 index 对应 line 中的字符
   *        把字符染成colormap[规则名字]
   * */
//   printf(1, "beautify\n");
   char* chs = concat_file(); // 把所有行的字符合成到一起
//   printf(1, "%s\n", chs);
   uint* colors = malloc(strlen(chs) * sizeof(uint));
   memset(colors, (BLACK << 4) | WHITE, strlen(chs) * sizeof(uint));
   int error; uint color_f; int i, j;
   char* syntax_color; // 存放 hashmap 中查到的颜色字符串
   int_node *idx, *jdx, *reg, *key;
   list *regex; char* creg;
   // 遍历所有 keyword
   for (key = syntax_keys->head; key != NULL; key = key->next) {
//     printf(1, "key: %s\n", (char*) key->data);
     hashmap_get(regex_map, (char*) key->data, (void**)(&regex));
     // 遍历 keyword 对应的所有正则表达式
     for (reg = regex->head; reg != NULL; reg = reg->next) {
       // 编译正则
       creg = (char*) reg->data;
       printf(1, "pattern: %s, ", creg);
       re_t pattern = re_compile(creg);
       list *match_length = new_list();
       list *matches = re_match_all(pattern, chs, match_length); // 匹配正则
       // 拿到 hashmap 中对应的颜色
       error = hashmap_get(colormap, (char*) key->data, (void*)(&syntax_color));
       printf(1, "color: %s\n", syntax_color);
       if (error != MAP_OK) color_f = DEFAULT_COLOR; // 找不到，用默认色
       // 是数字（十进制）就直接放进去，否则查颜色表
       else color_f = getcolor((syntax_color[0] >= '0' && syntax_color[0] <= '9') ?
           atoi(syntax_color) : find_color(syntax_color), BLACK);
       // 遍历所有匹配到的字符的 index，改颜色
       for (idx = matches->head, jdx = match_length->head; (idx != NULL && jdx != NULL);
        idx = idx->next, jdx = jdx->next) {
         printf(1, "|%c-%c|: %d, ", chs[idx->data], chs[idx->data + *(int*)(jdx->data)], color_f);
         memset(colors + idx->data, color_f, *(int*)(jdx->data) * sizeof(colors[0])); // 把idx后面jdx长度的字符颜色都改一下
       }
       free(match_length);
       free(matches);
       printf(1, "\n");
     }
   }
//  printf(1, "re end\n");

  // 将颜色信息同步到每行的 colors
   i = 0;
   for (line* l = tx.head; l != NULL; l = l->next) {
     j = 0;
     while (l->chs[j] != '\0' && j < l->n)
     {
      while(chs[i] == '\n' && j < l->n)
        i++;
      l->colors[j++] = colors[i++];
     }
   }

   free(chs);
   free(colors);
//   while (1);
}