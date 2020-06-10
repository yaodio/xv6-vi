// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "console.h"

static void consputc(int);

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

// 打印整数xx，指定进制为base，sign为正负号
static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// 打印到控制台上，不同于printf，后者可打印到指定描述符
// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd': // 十进制有符号整数
      printint(*argp++, 10, 1);
      break;
    case 'x': // 十六进制无符号整数
    case 'p': // 十六进制无符号整数（指针地址）
      printint(*argp++, 16, 0);
      break;
    case 's': // 字符串
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%': // 输出百分号%
      consputc('%');
      break;
    default:  // 其他未知情况输出百分号加上该字符%c
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];
  
  cli();
  cons.locking = 0;
  cprintf("cpu%d: panic: ", cpu->id);
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

// 获取光标位置
int
getcurpos(void)
{
  int pos;
  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);
  return pos;
}

// 设置光标位置
void
setcurpos(int pos, int c)
{
  // 范围检查
  if(pos < 0)
    pos = 0;
  else if(pos >= MAX_CHAR)
    pos = MAX_CHAR - 1;

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = (c&0xff) | 0x0700;
}

// 清屏
void
clearscreen(void)
{
  memset(crt, 0, sizeof(crt[0])*MAX_CHAR);
  setcurpos(0, ' ');
}

// 备份当前屏幕上的所有字符
void
backupscreen(ushort *backup, int nbytes)
{
  memmove(backup, crt, nbytes);
}

// 恢复屏幕内容
void
recoverscreen(ushort *backup, int nbytes)
{
  clearscreen();
  memmove(crt, backup, nbytes);
  setcurpos(nbytes / sizeof(crt[0]), ' '); // crt中的字符为ushort类型，占2字节
}

// 向屏幕输出1个字符
static void
cgaputc(int c)
{
  int pos = getcurpos();

  // 回车键 加上这一行剩余的空白数，即pos移到下一行行首
  if(c == '\n')
    pos += SCREEN_WIDTH - pos%SCREEN_WIDTH;
  // 退格键 pos退1个
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } else
    crt[pos++] = (c&0xff) | 0x0700;  // black on white
  
  if((pos/SCREEN_WIDTH) >= SCREEN_HEIGHT){  // Scroll up.
    memmove(crt, crt+SCREEN_WIDTH, sizeof(crt[0])*(SCREEN_HEIGHT-1)*SCREEN_WIDTH);
    pos -= SCREEN_WIDTH;
    memset(crt+pos, 0, sizeof(crt[0])*(MAX_CHAR - pos));
  }
  
  setcurpos(pos, ' ');
}

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else
    uartputc(c);
  cgaputc(c);
}

#define INPUT_BUF 128
struct {
  struct spinlock lock;
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

static int showflag = 1; // 为0时不打印到屏幕上
static int bufflag = 1;  // 为0时不缓存键盘输入，马上读取1个字符

// 设置bufflag和showflag
void
setflag(int sf, int bf)
{
  showflag = sf;
  bufflag = bf;
}

// 控制台的键盘输入中断处理函数（见trap.c）
void
consoleintr(int (*getc)(void))
{
  int c;

  acquire(&input.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      procdump();
      break;
    case C('R'):  // Sleeping process listing.
      print_sleeping();
      break;
    case C('U'):  // Kill line. 删除一整行
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace ctrl+H等效于退格键
      if(input.e != input.w){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    default:
      if(c != 0 && input.e-input.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        if(showflag)
          consputc(c);
        if(!bufflag || c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&input.lock);
}

// 处理定向到控制台的输入
int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&input.lock);
  while(n > 0){
    while(input.r == input.w){
      if(proc->killed){
        release(&input.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &input.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&input.lock);
  ilock(ip);

  return target - n;
}

// 处理定向到控制台的输出，如printf
int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

// 控制台初始化
void
consoleinit(void)
{
  initlock(&cons.lock, "console");
  initlock(&input.lock, "input");
  
  // 指定定向到控制台的输入输出处理函数
  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  picenable(IRQ_KBD);
  ioapicenable(IRQ_KBD, 0);
}

