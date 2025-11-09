#include "St1xMenu.h"
#include "u8g2.h"
#include "St1xKey.h"
#include "St1xStatic.h"
#include "St1xCalibrationSystem.h"
#include <stdio.h>
#include <string.h>

// 全局菜单上下文
static MenuContext g_menuCtx;

// 菜单状态变量
extern uint8_t menu_active;  // 菜单是否处于活动状态

// 校准准备状态
static uint8_t calibration_ready = 0;  // 是否处于校准准备状态

// 自动返回主菜单时间（55秒）
#define AUTO_RETURN_TIME 55000

// 上次操作时间
static uint32_t last_operation_time = 0;

// 前向声明
void Menu_DefaultAction(void);
void Level1Item3Action(void);
void Level1Item3Display(void);
uint8_t is_static_display_mode(void);
void exit_static_display_mode(void);
void SubMenu3CalibrationAction(void);

// 示例子菜单项
MenuItem subMenu1Items[] = {
    {"Sub1-Item1", Menu_DefaultAction, NULL, 0},
    {"Sub1-Item2", Menu_DefaultAction, NULL, 0},
    {"Sub1-Item3", Menu_DefaultAction, NULL, 0}
};

MenuItem subMenu2Items[] = {
    {"Sub2-Item1", Menu_DefaultAction, NULL, 0},
    {"Sub2-Item2", Menu_DefaultAction, NULL, 0}
};

MenuItem subMenu3Items[] = {
    {"Temperature Calib", SubMenu3CalibrationAction, NULL, 0},
    {"Sub3-Item2", Menu_DefaultAction, NULL, 0},
    {"Sub3-Item3", Menu_DefaultAction, NULL, 0},
    {"Sub3-Item4", Menu_DefaultAction, NULL, 0}
};

// 二级菜单项
MenuItem level2MenuItems[] = {
    {"SubMenu1", NULL, subMenu1Items, 3},
    {"SubMenu2", NULL, subMenu2Items, 2},
    {"Temperature Calib", SubMenu3CalibrationAction, NULL, 0}
};

// 一级菜单项
MenuItem rootMenuItems[] = {
    {"Level1-Item1", Menu_DefaultAction, NULL, 0},
    {"Level1-Item2", Menu_DefaultAction, NULL, 0},
    {"Level2Menu", NULL, level2MenuItems, 3},
    {"Level1-Item3", Level1Item3Action, NULL, 0}
};

/**
 * @brief 菜单初始化函数
 * @param ctx 菜单上下文指针
 * @param rootMenu 根菜单项数组
 * @param rootMenuCount 根菜单项数量
 */
void Menu_Init(MenuContext* ctx, MenuItem* rootMenu, uint8_t rootMenuCount) {
    ctx->currentMenu = rootMenu;
    ctx->currentItemIndex = 0;
    ctx->menuLevel = 0;
    ctx->menuStack[0] = rootMenu;
}

/**
 * @brief 默认菜单项动作函数
 */
void Menu_DefaultAction(void) {
    // 默认动作，可以被具体实现替换
}

/**
 * @brief 温度校准菜单项动作函数
 */
void SubMenu3CalibrationAction(void) {
    // 使用新的状态机模式进入校准模式
    // 电压检查由校准系统内部处理，这里不需要重复检查
    CalibrationSystem_Start();
}

// 传感器显示模式标志
static uint8_t static_display_mode = 0;
static uint32_t last_static_display_time = 0;
#define STATIC_DISPLAY_INTERVAL 50  // 50ms显示间隔

/**
 * @brief Level1-Item3 菜单项动作函数
 */
void Level1Item3Action(void) {
    // 调用静态传感器显示初始化
    St1xStatic_Action();
    
    // 设置为传感器显示模式
    static_display_mode = 1;
    last_static_display_time = HAL_GetTick();
}

/**
 * @brief 检查是否处于静态显示模式
 */
uint8_t is_static_display_mode(void) {
    return static_display_mode;
}

/**
 * @brief 退出静态显示模式
 */
void exit_static_display_mode(void) {
    static_display_mode = 0;
}

/**
 * @brief 静态数据显示处理函数
 */
void Level1Item3Display(void) {
    extern u8g2_t u8g2;
    uint32_t current_time = HAL_GetTick();
    
    // 按设定间隔刷新显示
    if ((current_time - last_static_display_time) >= STATIC_DISPLAY_INTERVAL) {
        // 显示传感器数据
        St1xStatic_DisplayData(&u8g2);
        last_static_display_time = current_time;
    }
}

/**
 * @brief 处理菜单输入
 * @param ctx 菜单上下文指针
 * @param direction 输入方向
 */
void Menu_HandleInput(MenuContext* ctx, MenuDirection direction) {
    uint8_t itemCount = 0;
    
    // 获取当前菜单项数量
    if (ctx->menuLevel == 0) {
        itemCount = sizeof(rootMenuItems) / sizeof(MenuItem);
    } else {
        itemCount = ctx->menuStack[ctx->menuLevel]->subMenuCount;
    }
    
    switch (direction) {
        case MENU_DIRECTION_UP:
            if (ctx->currentItemIndex > 0) {
                ctx->currentItemIndex--;
            } else {
                // 光标在第一项时向上移动，跳转到最后一项
                ctx->currentItemIndex = itemCount - 1;
            }
            break;
            
        case MENU_DIRECTION_DOWN:
            if (ctx->currentItemIndex < itemCount - 1) {
                ctx->currentItemIndex++;
            } else {
                // 光标在最后一项时向下移动，跳转到第一项
                ctx->currentItemIndex = 0;
            }
            break;
            
        case MENU_DIRECTION_ENTER:
            // 检查是否有子菜单
            if (ctx->menuLevel < MAX_MENU_LEVELS - 1) {
                MenuItem* currentItem = &ctx->currentMenu[ctx->currentItemIndex];
                if (currentItem->subMenu != NULL) {
                    // 进入子菜单
                    ctx->menuLevel++;
                    ctx->menuStack[ctx->menuLevel] = currentItem->subMenu;
                    ctx->stackIndex[ctx->menuLevel-1] = ctx->currentItemIndex;
                    ctx->currentMenu = currentItem->subMenu;
                    ctx->currentItemIndex = 0;
                } else if (currentItem->action != NULL) {
                    // 执行动作
                    currentItem->action();
                }
            } else if (ctx->currentMenu[ctx->currentItemIndex].action != NULL) {
                // 在最深层菜单执行动作
                ctx->currentMenu[ctx->currentItemIndex].action();
            }
            break;
            
        case MENU_DIRECTION_BACK:
            if (ctx->menuLevel > 0) {
                // 返回上级菜单
                ctx->menuLevel--;
                if (ctx->menuLevel == 0) {
                    ctx->currentMenu = rootMenuItems;
                } else {
                    ctx->currentMenu = ctx->menuStack[ctx->menuLevel];
                }
                ctx->currentItemIndex = ctx->stackIndex[ctx->menuLevel];
            }
            break;
    }
}

/**
 * @brief 显示菜单
 * @param ctx 菜单上下文指针
 * @param u8g2 u8g2显示对象指针
 */
void Menu_Display(MenuContext* ctx, u8g2_t* u8g2) {
    uint8_t i;
    uint8_t itemCount = 0;
    uint8_t startY = 20;
    uint8_t lineHeight = 12;
    
    // 清除显示缓冲区
    u8g2_ClearBuffer(u8g2);
    
    // 显示标题
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 0, 10, "MENU:");
    
    // 获取当前菜单项数量
    if (ctx->menuLevel == 0) {
        itemCount = sizeof(rootMenuItems) / sizeof(MenuItem);
    } else {
        itemCount = ctx->menuStack[ctx->menuLevel]->subMenuCount;
    }
    
    // 显示菜单项
    for (i = 0; i < itemCount && i < MAX_MENU_ITEMS; i++) {
        uint8_t y = startY + i * lineHeight;
        
        // 如果是当前选中项，显示箭头
        if (i == ctx->currentItemIndex) {
            u8g2_DrawStr(u8g2, 0, y, ">");
        }
        
        // 显示菜单项名称
        u8g2_DrawStr(u8g2, 10, y, ctx->currentMenu[i].name);
        
        // 如果有子菜单，显示标记
        if (ctx->currentMenu[i].subMenu != NULL) {
            u8g2_DrawStr(u8g2, 110, y, ">");
        }
    }
    
    // 显示层级信息
    char levelStr[20];
    sprintf(levelStr, "Level: %d", ctx->menuLevel + 1);
    u8g2_DrawStr(u8g2, 0, 64, levelStr);
    
    // 发送缓冲区内容到显示屏
    u8g2_SendBuffer(u8g2);
}

/**
 * @brief 初始化菜单系统
 */
void Menu_InitSystem(void) {
    // 初始化菜单
    Menu_Init(&g_menuCtx, rootMenuItems, sizeof(rootMenuItems) / sizeof(MenuItem));
    
    // 标记菜单为活动状态
    menu_active = 1;
    
    // 记录操作时间
    last_operation_time = HAL_GetTick();
    
    // 初始化按键模块
    Key_Init();
}

/**
 * @brief 菜单系统任务处理函数（非阻塞方式）
 * @return 1表示菜单仍在运行，0表示菜单已退出
 */
uint8_t Menu_Process(void) {
    // 如果菜单不处于活动状态，直接返回
    if (!menu_active) {
        return 0;
    }
    
    // 检查是否超时，自动返回主菜单
    // 注意：在校准模式下禁用自动返回功能
    if (!St1xCalibration_IsInProgress() && (HAL_GetTick() - last_operation_time) >= AUTO_RETURN_TIME) {
        // 直接退出菜单系统
        menu_active = 0;
        return 0;
    }
    
    // 扫描按键
    KeyType key = Key_Scan();
    
    // 如果有按键操作，更新操作时间
    if (key != KEY_NONE) {
        last_operation_time = HAL_GetTick();
    }
    
    // 检查是否处于校准模式
    if (St1xCalibration_IsInProgress()) {
        // 在校准模式下，按键由校准模块处理
        St1xCalibration_HandleKey(key);
    } else {
        // 正常菜单模式下的按键处理
        switch (key) {
            case KEY_UP:
                if (is_static_display_mode()) {
                    // 在静态显示模式下，UP键无操作
                    break;
                }
                Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_DOWN); // 向下移动
                break;
                
            case KEY_DOWN:
                if (is_static_display_mode()) {
                    // 在静态显示模式下，DOWN键无操作
                    break;
                }
                Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_UP); // 向上移动
                break;
                
            case KEY_MODE:
                if (is_static_display_mode()) {
                    // 在静态显示模式下，KEY_MODE键用于退出
                    exit_static_display_mode();
                    // 同时执行加热控制逻辑
                    handleHeatingControl();
                    break;
                }
                // 在菜单中，KEY_MODE按键同时用于菜单导航和加热控制
                // 调用加热控制函数
                handleHeatingControl();
                // 执行菜单导航
                Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_ENTER);
                break;
                
            case KEY_MODE_LONG:
                if (is_static_display_mode()) {
                    // 在静态显示模式下，长按KEY_MODE键用于退出
                    exit_static_display_mode();
                    break;
                }
                if (g_menuCtx.menuLevel > 0) {
                    Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_BACK);
                } else {
                    // 已经在根菜单，退出菜单系统
                    menu_active = 0;
                    return 0;
                }
                break;
                
            default:
                break;
        }
    }
    
    // 获取U8G2对象
    extern u8g2_t u8g2;
    
    // 检查是否处于校准模式
    if (St1xCalibration_IsInProgress()) {
        // 在校准模式下，显示校准界面
        St1xCalibration_MainLoop(&u8g2);
    } else if (St1xCalibration_IsInProgress()) {
        // 显示校准界面（由校准模块处理）
        St1xCalibration_MainLoop(&u8g2);
    } else if (is_static_display_mode()) {
        // 处理静态数据显示
        Level1Item3Display();
    } else {
        // 显示菜单
        Menu_Display(&g_menuCtx, &u8g2);
    }
    
    return 1; // 菜单仍在运行
}