#include "vulib.h"

#include "../user.h"
#include "../stat.h"
#include "../fcntl.h"
#include "color.h"

// 根据传入的字符数组，构造双向链表，每个节点是一行
line*
newlines(uchar *chs, uint n)
{
  int i;
  line *l = (line*)malloc(sizeof(line));
  memset(l->chs, '\0', MAX_COL);
  memset(l->colors, DEFAULT_COLOR, MAX_COL);
  l->n = 0;
  l->paragraph = 0;
  l->prev = l->next = NULL;

  // 空行
  if(chs == NULL || n == 0)
    return l;

  for(i = 0; i < n; i++){
    // 换行
    if(chs[i] == '\n'){
      l->next = newlines(chs+i+1, n-i-1);
      l->next->prev = l;
      break;
    }
    // 写满一行，则下一行与该行同段
    if(i >= MAX_COL){
      l->next = newlines(chs+i, n-i);
      l->next->prev = l;
      l->paragraph = 1;
      break;
    }
    // 填写一行中
    l->chs[i] = chs[i];
    l->n++;
  }

  return l;
}

void
setline(line *l, uchar *chs, int n, uchar color)
{
  memset(l->chs, '\0', MAX_COL);
  memset(l->colors, color, MAX_COL);
  memmove(l->chs, chs, n);
  l->n = n;
}

void
cleanline(line *l)
{
  setline(l, NULL, 0, DEFAULT_COLOR);
}

// 从指定的文件路径中读取所有内容，并组织成行结构（双向链表），出错时返回-1
int
readtext(char *path, struct text* txx)
{
  // NOTE: 传递指针以备将来可能同时打开多个 text
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  uint nbytes;            // 文件大小（字节数）
  uchar *chs;             // 文件中的所有字符

  // 路径存在且可被打开
  if(path != NULL && (fd = open(path, O_RDONLY)) >= 0){
    // 获取文件信息失败则退出
    if(fstat(fd, &st) < 0){
      printf(2, "editor: cannot stat %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 否则检查文件信息，是否为文件（可能是目录）
    if(st.type != T_FILE){
      printf(2, "editor: cannot edit a directory: %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 走到这里说明成功打开了一个文件，读取其中的所有字符
    nbytes = st.size;
    chs = (uchar*)malloc(nbytes);
    read(fd, chs, nbytes);
//    printf(1, "open file succeed\n%s", chs);
    close(fd); // 与open匹配
    txx->exist = 1;
  }
    // 路径不存在
  else{
    nbytes = 0;
    chs = NULL;
    txx->exist = 0;
  }

  txx->path = path;
  // 将文件内容组织成行结构，并用指针进行标记
  txx->head = txx->tail = newlines(chs, nbytes);

  // 定位尾行
  while(txx->tail->next != NULL){
    txx->tail = txx->tail->next;
  }

  return 0;
}

int
writetext(char* path, line* l)
{
  // 输出到path
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息

  // 路径存在且可被打开
  if(path != NULL && (fd = open(path, O_WRONLY | O_CREATE)) >= 0){
    // 获取文件信息失败则退出
    if(fstat(fd, &st) < 0){
      // printf(2, "editor: cannot stat %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 否则检查文件信息，是否为文件（可能是目录）
    // 在读文件的时候已经检查，不应有错
    if(st.type != T_FILE){
      // printf(2, "editor: cannot edit a directory: %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 写入文件
    while (l != NULL) {
      write(fd, l->chs, l->n);
      if (l->paragraph != 1)
        write(fd, "\n", 1);
      l = l->next;
    }
    close(fd); // 与open匹配
    return 0;
  }
  // 路径不存在
  else return -1;
}

char*
getfilename(char *path)
{
  int i, len;
  char* name;

  if(path == NULL)
    return NULL;
  
  len = strlen(path);
  for(i = len - 1; i>=0 && path[i] != '/'; i--)
    ;
  
  name = (char*)malloc(len-i);
  memmove(name, path+i+1, len-i);
  return name;
}