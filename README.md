# vi for xv6

> xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix Version 6 (v6).  xv6 loosely follows the structure and style of v6, but is implemented for a modern x86-based multiprocessor using ANSI C.

This is a tiny vi for [xv6](https://github.com/mit-pdos/xv6-public.git).

## Installation

```bash
# ubuntu or debian
git clone https://gitee.com/yaodio/xv6-vi.git
sudo apt-get install qemu
cd xv6
make
make qemu

# if "fatal error: sys/cdefs.h", try:
sudo apt-get purge libc6-dev
sudo apt-get install libc6-dev
sudo apt-get install libc6-dev-i386
```

QEMU usage:
- `CTRL+A+X` to quit.
- `CTRL+A+C` to debug.

## Feature

- view mode
  - [x] cursor movement
- insert mode
  - [x] insert and delete (i, a and backspace)
  - [x] jump to line begin (0)
  - [x] jump to line end ($)
  - [x] jump to screen begin (H)
  - [x] jump to last line begin of screen (L)
  - [x] quit to normal (ESC)
- baseline mode
  - [x] write file to the origin or given path (:w [path])
  - [x] quit (:q)
  - [x] write file to the origin or given path and quit (:wq [path])
  - [x] force to quit (:q!)
  - [x] show help page (:h)
- syntax highlight
  - [x] read syntax highlight rule file [LANG].vi
  - [x] highlight regex strings by file postfix

## Contribute

This project is created by

- yaodio
- sexyboy
- [stephen ark](https://github.com/StephenArk30)

We welcome any improvement! Feel free to fork and make pull request. If you don't know how to start, you can follow this (if you are new to git, you may need to do some search):

1. Create an issue, describe what feature you want to add. It tells others you are working on this feature and avoid repeat work. Also, you should look up issues and ensure you are not repeating other's work
2. Fork this project
3. Clone **YOUR FORKED PROJECT** to your local:

```bash
git clone https://github.com/{YOUR NAME}/xv6-vi.git
```

4. Set this project as upstream and checkout to a new branch where you add your new feature:

```bash
cd xv6
git remote add upstream https://gitee.com/yaodio/xv6-vi.git
git checkout {YOUR_BRANCH}
```

​	The "upstream" is for syncing with the origin repo. When the upstream repo is updated, you can sync your forked repo by:

```bash
git fetch upstream
git merge upstream/{YOUR_BRANCH}
```

​	You should always do this before making changes and makeing pull request.

5. Install qemu, make if you haven't (see Installation)

6. Make your changes. Use ``make && make qemu`` to enter xv6

7. Type ``vi {file}`` to your xv6 window and you should see vi running
8. When you finish your work, push to your remote by ``git push``
9. Go to your project's "Pull requests" tab, click "New pull request"
10. Select your branch and click "Create new pull request". You are now finish your great contribution!

## Library repo

Open source libraries used in project:

- [tiny-regex-c](https://github.com/kokke/tiny-regex-c)
- [c_hashmap](https://github.com/petewarden/c_hashmap)

Thanks to their great work.