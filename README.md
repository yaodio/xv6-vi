# vi for xv6

> xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix Version 6 (v6).  xv6 loosely follows the structure and style of v6, but is implemented for a modern x86-based multiprocessor using ANSI C.

This is a tiny vi for xv6. See editor.c.

## Installation

```bash
# ubuntu or debian
git clone https://gitee.com/yaodio/xv6.git
sudo apt-get install qemu
cd xv6
make
make qemu-nox

# if "fatal error: sys/cdefs.h", try:
sudo apt-get purge libc6-dev
sudo apt-get install libc6-dev
sudo apt-get install libc6-dev-i386
```

QEMU usage:
- `CTRL+A+X` to quit.
- `CTRL+A+C` to debug.

## Lookup Table

### Variables and Structs
- SCREEN_WIDTH, SCREEN_HEIGHT: 屏幕宽高 (80, 25)
- cur: 光标变量
    - cur.row, cur.col: 光标行列
    - cur.l: 光标指向的行节点

### Functions
- curup, curdown, curleft, curright, curto: 移动光标
- showcur: 显示光标
- getfirstline: 根据光标所在屏幕的行数，往前推出屏幕上指向的第0行
- maxcoldown: 光标下移屏幕滚动
- newlines: 根据传入的字符数组，构造双向链表，每个节点是一行
- printline: 在屏幕上第row行打印指定的行
- printlines: 从屏幕上第 row 行开始打印指定行 l 以及其后面的行，直到屏幕写满
- insertc: 在光标处插入字符