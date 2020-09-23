#include "vi.h"

extern text tx;
map_t regex_map, colormap;
list *syntax_keys;

char colorname[16][32] = {"BLACK", "BLUE", "GREEN", "CYAN", "RED", "PURPLE", "BROWN", "GREY", "DARK_GREY", "LIGHT_BLUE",
                          "LIGHT_GREEN", "LIGHT_CYAN", "LIGHT_RED", "LIGHT_PURPLE", "YELLOW", "WHITE"};
uint colorint[16] = {BLACK, BLUE, GREEN, CYAN, RED, PURPLE, BROWN, GREY, DARK_GREY, LIGHT_BLUE,
                    LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED, LIGHT_PURPLE, YELLOW, WHITE};

// 组合文本色tcolor和背景色bcolor
uchar getcolor(uchar tcolor, uchar bcolor)
{
  return (bcolor << 4) | tcolor;
}

// 给字符c涂上颜色color（color已组合了文本色和背景色信息）
ushort
paintc(uchar c, uchar color)
{
  return (color << 8) | c;
}

// 给一行设置同样的颜色
void
paintl(line *l, uchar color)
{
  memset(l->colors, color, MAX_COL);
}

// 返回下一个单词（空格分开）
char*
sscanf(uchar *chs, int *offset)
{
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
  *offset += i;

  // 跳过空格，不会跳过换行
  while (*(chs+*offset) != '\0' &&
      (*(chs+*offset) == ' ' || *(chs+*offset) == '\t'))
    *offset += 1;
  return res;
}

void
read_syntax(void)
{
  regex_map = hashmap_new();
  colormap = hashmap_new();
  syntax_keys = new_list();

  int i;
  for (i = (int)strlen(tx.path)-1; i >= 0; i--)
    if (tx.path[i] == '.') break;
  if (i < 0) return;
  char *vi_file = malloc((strlen(tx.path) - i + 3) * sizeof(char));
  memmove(vi_file, tx.path + i + 1, (int)strlen(tx.path) - i - 1);
  memmove(vi_file + strlen(tx.path) - i - 1, ".vi\0", 4);
  logging((TRACE_FILE, "[read_syntax] "), (TRACE_FILE, "opening highlight rule %s\n", vi_file));

  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  uchar *chs = NULL; int offset = 0;
  if ((fd = open(vi_file, O_RDONLY)) >= 0) {
    if (fstat(fd, &st) < 0 || st.type != T_FILE)
      goto read_syntax_end; // 打开文件失败

    chs = (uchar *) malloc(st.size);
    read(fd, chs, st.size); // 读入高亮规则
    int error;
    while (*(chs+offset) != '\0') {
      char* type = sscanf(chs, &offset); // 类型，keyword 或 hi
      if (strcmp(type, "keyword") == 0) { // keyword
        char* key = sscanf(chs, &offset); // 键
        if (*(chs+offset) == '\0') break;
        push_back(syntax_keys, (int) key);
        char* regex; // 一个键有多个正则表达式
        for (regex = sscanf(chs, &offset); *regex != '\n'; regex = sscanf(chs, &offset)) {
          struct list* lst; // 正则表达式列表
          error = hashmap_get(regex_map, key, (void**)(&lst));
          if (error != MAP_OK) lst  = new_list(); // 如果之前没有，则新建
          push_back(lst, (int) regex); // 添加本条正则
          if (error != MAP_OK)
            hashmap_put(regex_map, key, lst);
          if (*(chs+offset) == '\0') break;
        }
      } else if (strcmp(type, "hi") == 0) { // hi
        char* key = sscanf(chs, &offset); // 键
        if (*(chs+offset) == '\0') break;
        char* color = sscanf(chs, &offset); // 一个键对应一个颜色
        hashmap_put(colormap, key, color);
        offset++; // 跳过换行
      }
    }
    free(chs);
  } else {
//    printf(2, "cannot open file %s\n", vi_file);
  }
read_syntax_end:
  close(fd); // 与open匹配
  free(vi_file);
}

// 返回所有行拼接的字符串（不含换行）
char*
concat_file(void)
{
  int line_num = 0;
  for (line* l = tx.head; l != NULL; l = l->next)
    if(!l->paragraph)
      line_num++; // 如果不是一段，则应加入换行符
  char* chs = malloc((tx.nchar + line_num) * sizeof(char));
  int i = 0, j;
  for (line* l = tx.head; l != NULL; l = l->next) {
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
find_color(char *cname)
{
  int l = sizeof(colorname) / sizeof(colorname[0]);
  for (int i = 0; i < l; i++) {
    if (strcmp(cname, colorname[i]) == 0) {
      return colorint[i];
    }
  }
  return DEFAULT_COLOR;
}

// 根据文件类型来着色
void
beautify(void)
{
  if (syntax_keys->size == 0) return;
  char* chs = concat_file(); // 把所有行的字符合成到一起
  uint* colors = malloc(strlen(chs) * sizeof(uint));
  memset(colors, getcolor(WHITE, BLACK), strlen(chs) * sizeof(uint));
  int error; uint color_f; int i, j;
  char* syntax_color; // 存放 hashmap 中查到的颜色字符串
  int_node *idx, *jdx, *reg, *key;
  list *regex; char* creg;
  // 遍历所有 keyword
  for (key = syntax_keys->head; key != NULL; key = key->next) {
   hashmap_get(regex_map, (char*) key->data, (void**)(&regex));
   // 遍历 keyword 对应的所有正则表达式
   for (reg = regex->head; reg != NULL; reg = reg->next) {
     // 编译正则
     creg = (char*) reg->data;
     re_t pattern = re_compile(creg); // FIXME: 应当把正则编译放在文件读取阶段
     list *match_length = new_list();
     list *matches = re_match_all(pattern, chs, match_length); // 匹配正则
     // 拿到 hashmap 中对应的颜色
     error = hashmap_get(colormap, (char*) key->data, (void*)(&syntax_color));
     if (error != MAP_OK) color_f = DEFAULT_COLOR; // 找不到，用默认色
     // 是数字（十进制）就直接放进去，否则查颜色表
     else color_f = getcolor((syntax_color[0] >= '0' && syntax_color[0] <= '9') ?
         atoi(syntax_color) : find_color(syntax_color), BLACK);
     // 遍历所有匹配到的字符的 index，改颜色
     for (idx = matches->head, jdx = match_length->head; (idx != NULL && jdx != NULL);
      idx = idx->next, jdx = jdx->next) {
       memset(colors + idx->data, color_f, *(int*)(jdx->data) * sizeof(colors[0])); // 把idx后面jdx长度的字符颜色都改一下
     }
     free(match_length);
     free(matches);
   }
  }

  // 将颜色信息同步到每行的 colors
  i = 0;
  for (line* l = tx.head; l != NULL; l = l->next) {
   j = 0;
   while (l->chs[j] != '\0' && j < l->n)
   {
    while(chs[i] == '\n' && j < l->n) i++; // 跳过换行
    l->colors[j++] = colors[i++];
   }
  }

  free(chs);
  free(colors);
}
