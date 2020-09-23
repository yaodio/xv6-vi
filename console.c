// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
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
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
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
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
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
setcurpos(int pos)
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
}

// 在屏幕的 pos 位置处设置字符（不移动光标），传入的 int c 仅低 16 位有效
// 其中从右往左数：
// 0~7   字符的 ascii 码值
// 8~11  文本色
// 12~15 背景色
void
showc(int pos, int c)
{
  crt[pos] = c;
}

static void
cgaputc(int c)
{
  int pos = getcurpos();

  // 回车键 加上这一行剩余的空白数，即 pos 移到下一行行首
  if(c == '\n')
    pos += SCREEN_WIDTH - pos%SCREEN_WIDTH;
  else if(c == BACKSPACE){ // 退格键 pos 退 1 个
    if(pos > 0) --pos;
    crt[pos] = ' ';
  } else showc(pos++, (c&0xff) | 0x0700);

  if(pos < 0 || pos > MAX_CHAR)
    panic("pos under/overflow");

  if((pos/SCREEN_WIDTH) >= (SCREEN_HEIGHT-1)){  // Scroll up.
    memmove(crt, crt+SCREEN_WIDTH, sizeof(crt[0])*(SCREEN_HEIGHT-2)*SCREEN_WIDTH);
    pos -= SCREEN_WIDTH;
    memset(crt+pos, 0, sizeof(crt[0])*((SCREEN_HEIGHT-1)*SCREEN_WIDTH - pos));
  }

  setcurpos(pos);
  showc(pos, (crt[pos]&0xff) | 0x0700);
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
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

static int showflag = 1;      // 为 0 时不打印到屏幕上
static int bufflag = 1;       // 为 0 时不缓存键盘输入，马上读取 1 个字符
static int backspaceflag = 1; // 为 0 时不拦截退格键

// 设置 showflag, bufflag, backspaceflag
void
setflag(int sf, int bf, int bsf)
{
  showflag = sf;
  bufflag = bf;
  backspaceflag = bsf;
}

void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if (backspaceflag) {
        if(input.e != input.w){
          input.e--;
          consputc(BACKSPACE);
        }
        break;
      }
    default:
      if(c != 0 && input.e-input.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        if (showflag) consputc(c);
        if (!bufflag || c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
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
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

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

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}

// 清屏
void
clearscreen(void)
{
  memset(crt, 0, sizeof(crt[0])*MAX_CHAR);
  setcurpos(0);
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
  int pos = nbytes / sizeof(crt[0]); // crt中的字符为ushort类型，占2字节
  clearscreen();
  memmove(crt, backup, nbytes);
  setcurpos(pos);
}
