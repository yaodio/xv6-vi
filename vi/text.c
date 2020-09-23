#include "vi.h"

// 从指定的文件路径中读取所有内容，并组织成行结构（双向链表），出错时返回-1
int
readtext(char *path, text *tx)
{
  // NOTE: 传递指针以备将来可能同时打开多个 text
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  uint nbytes;            // 文件大小（字节数）
  uchar *chs;             // 文件中的所有字符
  line *l;                // 迭代用的临时指针

  logging((TRACE_FILE, "[readtext] "), (TRACE_FILE, "opening %s\n", path));

  tx->path = (char*)malloc(MAX_COL);
  memset(tx->path, '\0', MAX_COL);
  // 输入了路径
  if(path != NULL)
    strcpy(tx->path, path);

  // 路径存在且可被打开
  if(path != NULL && (fd = open(path, O_RDONLY)) >= 0){
    logging((TRACE_FILE, "[readtext] "), (TRACE_FILE, "reading...\n"));
    // 获取文件信息失败则退出
    if(fstat(fd, &st) < 0){
      printf(2, "vi: cannot stat %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 否则检查文件信息，是否为文件（可能是目录）
    if(st.type != T_FILE){
      printf(2, "vi: cannot edit a directory: %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    logging((TRACE_FILE, "[readtext] "), (TRACE_FILE, "reading succeed\n"));
    // 走到这里说明成功打开了一个文件，读取其中的所有字符
    nbytes = st.size;
    chs = (uchar*)malloc(nbytes);
    read(fd, chs, nbytes);
    close(fd); // 与open匹配
    tx->exist = 1;
  }
  // 路径不存在
  else{
    nbytes = 0;
    chs = NULL;
    tx->exist = 0;
  }

  logging((TRACE_FILE, "[readtext] "), (TRACE_FILE, "creating new lines...\n"));
  // 将文件内容组织成行结构，并用head指针记录头节点
  l = tx->head = newlines(chs, nbytes);

  logging((TRACE_FILE, "[readtext] "), (TRACE_FILE, "counting characters...\n"));
  // 字符计数
  while(l->next != NULL){
    tx->nchar += l->n;
    l = l->next;
  }

  logging((TRACE_FILE, "[readtext] "), (TRACE_FILE, "creating new lines...\n"));
  if(chs)
    free(chs);
  logging((TRACE_FILE, "[readtext] "), ("%s loaded\n", path));

  return 0;
}

int
writetext(text *tx)
{
  // 输出到path
  int fd;                 // 文件描述符
  struct stat st;         // 文件信息
  line *l;

  // 路径存在且可被打开
  if(tx->path != NULL && (fd = open(tx->path, O_WRONLY | O_CREATE)) >= 0){
    // 获取文件信息失败则退出
    if(fstat(fd, &st) < 0){
      // printf(2, "vi: cannot stat %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 否则检查文件信息，是否为文件（可能是目录）
    // 在读文件的时候已经检查，不应有错
    if(st.type != T_FILE){
      // printf(2, "vi: cannot edit a directory: %s\n", path);
      close(fd); // 与open匹配
      return -1;
    }

    // 写入文件
    l = tx->head;
    while (l != NULL) {
      write(fd, l->chs, l->n);
      if (l->paragraph != 1 && l->next != NULL)
        write(fd, "\n", 1);
      l = l->next;
    }
    close(fd); // 与open匹配
    return 0;
  }
  // 路径不存在
  else return -1;
}

// 释放text的内存空间
void
freetx(text *tx)
{
  if(tx != NULL){
    if(tx->path)
      free(tx->path);
    if(tx->head)
      freelines(tx->head);
  }
}