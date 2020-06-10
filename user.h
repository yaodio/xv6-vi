struct stat;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

int getcurpos(void);        // 获取光标位置
int setcurpos(int, int);    // 设置光标位置, 以及该位置上的字符
int cls(void);              // 清屏
int bks(ushort*, int);      // 备份当前屏幕上的所有字符
int rcs(ushort*, int);      // 恢复屏幕内容
int consflag(int, int);     // 设置控制台的showflag和bufflag
int putcc(int, int);        // 在指定位置处打印字符，字符的低8～15位可携带颜色信息

// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
