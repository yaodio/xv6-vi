> xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix Version 6 (v6).  xv6 loosely follows the structure and style of v6, but is implemented for a modern x86-based multiprocessor using ANSI C.

# Installation

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
