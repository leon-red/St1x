#ifndef ST1XMENU_H_
#define ST1XMENU_H_

// 包含必要的头文件
#include "main.h"

// 前向声明u8g2_t结构体，避免包含整个u8g2.h头文件
struct u8g2_struct;
typedef struct u8g2_struct u8g2_t;

// 声明外部加热控制函数
extern void handleHeatingControl(void);

// 定义最大菜单层级和菜单项数
#define MAX_MENU_LEVELS 5
#define MAX_MENU_ITEMS 10

// 菜单方向枚举
typedef enum {
    MENU_DIRECTION_UP,
    MENU_DIRECTION_DOWN,
    MENU_DIRECTION_ENTER,
    MENU_DIRECTION_BACK
} MenuDirection;

// 菜单项结构体前向声明
typedef struct MenuItemStruct MenuItem;

// 菜单项结构体定义
struct MenuItemStruct {
    char* name;                    // 菜单项名称
    void (*action)(void);          // 动作函数指针
    MenuItem* subMenu;             // 子菜单指针
    uint8_t subMenuCount;          // 子菜单项数量
};

// 菜单上下文结构体
typedef struct {
    MenuItem* currentMenu;                    // 当前菜单
    uint8_t currentItemIndex;                 // 当前选中项索引
    uint8_t menuLevel;                        // 当前菜单层级
    MenuItem* menuStack[MAX_MENU_LEVELS];     // 菜单栈
    uint8_t stackIndex[MAX_MENU_LEVELS-1];    // 栈索引
} MenuContext;

// 函数声明
void Menu_Init(MenuContext* ctx, MenuItem* rootMenu, uint8_t rootMenuCount);
void Menu_HandleInput(MenuContext* ctx, MenuDirection direction);
void Menu_Display(MenuContext* ctx, u8g2_t* u8g2);
void Menu_DefaultAction(void);

// 新增的非阻塞式菜单函数
void Menu_InitSystem(void);
uint8_t Menu_Process(void);

#endif /* ST1XMENU_H_ */