#define BLACK           0x0                     // 黑
#define BLUE            0x1                     // 蓝
#define GREEN           0x2                     // 绿
#define CYAN            0x3                     // 青
#define RED             0x4                     // 红
#define PURPLE          0x5                     // 紫
#define BROWN           0x6                     // 棕
#define GREY            0x7                     // 灰
#define DARK_GREY       0x8                     // 深灰
#define LIGHT_BLUE      0x9                     // 浅蓝
#define LIGHT_GREEN     0xa                     // 浅绿
#define LIGHT_CYAN      0xb                     // 浅青
#define LIGHT_RED       0xc                     // 浅红
#define LIGHT_PURPLE    0xd                     // 浅紫
#define YELLOW          0xe                     // 黄
#define WHITE           0xf                     // 白

#define DEFAULT_COLOR   (BLACK << 4) | GREY     // 默认黑色背景灰色文本

// 颜色模式
#define UNCOLORED       0                       // 黑灰模式（不着色）
#define COLORFUL        1                       // 彩色模式