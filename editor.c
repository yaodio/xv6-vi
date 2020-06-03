#include "types.h"
#include "stat.h"
#include "user.h"

void
editor(char *path) {
    // TODO
    printf(1, "editor: %s\n", path);
}

int
main(int argc, char *argv[])
{
    // 命令行输入：editor path
    // 没有输入文件路径path，不允许使用editor
    if(argc <= 1) {
        printf(1, "editor: lack of file path!\n");
        exit();
    }

    editor(argv[1]);
    exit();
}