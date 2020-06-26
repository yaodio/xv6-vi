#ifndef VI_XV6_VI_H
#define VI_XV6_VI_H

// xv6系统头文件
#include "../types.h"
#include "../stat.h"
#include "../user.h"
#include "../fcntl.h"
#include "../kbd.h"
#include "../console.h"

// vi基本宏定义
#define NULL          0                   // 空指针
#define MAX_COL       SCREEN_WIDTH        // 每行最大字符数设为屏幕宽度
#define BASE_ROW      (SCREEN_HEIGHT-1)   // 底线行的行号
#define TAB_WIDTH     4                   // \t的宽度

// 程序各部分组件的头文件，若要添加组件，将头文件统一写到这里，注意顺序
#include "line.h"       // 行结构体与行相关操作
#include "text.h"       // 文本结构体与读写文本操作
#include "cursor.h"     // 光标结构体与光标操作

#include "vulib.h"      // 通用函数功能
#include "color.h"      // 颜色模块
#include "re.h"         // 正则解析模块
#include "stl.h"        // hashmap模块

#include "help.h"       // help页面
#include "baseline.h"   // 底线模式操作
#include "editmode.h"   // 编辑模式操作

#endif // VI_XV6_VI_H