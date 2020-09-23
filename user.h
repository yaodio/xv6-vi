#ifndef XV6_USER_H
#define XV6_USER_H

struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

/* custom syscall */
int getcurpos(void);        // 获取光标位置
int setcurpos(int);         // 设置光标位置
int putcc(int, int);        // 在指定位置处打印字符，字符的低8～15位可携带颜色信息
int consflag(int, int, int);// 设置控制台的 showflag、bufflag、backspaceflag
int cls(void);              // 清屏
int bks(ushort*, int);      // 备份当前屏幕上的所有字符
int rcs(ushort*, int);      // 恢复屏幕内容

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

#endif // XV6_USER_H