#include "St1xMenu.h"
#include "u8g2.h"
#include "St1xKey.h"
#include "St1xStatic.h"
#include "St1xCalibrationSystem.h"
#include "St1xADC.h"
#include "Buzzer.h"
#include <stdio.h>
#include <math.h>
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

// ==================== 显示系统相关变量 ====================

// 显示温度滤波系统
#define DISPLAY_FILTER_SIZE 8   // 显示用温度滤波器窗口大小（平滑显示）
static float display_temperature_buffer[DISPLAY_FILTER_SIZE] = {0}; // 显示温度缓冲区
static uint8_t display_filter_index = 0;                             // 显示缓冲区索引
static uint8_t display_filter_initialized = 0;                      // 显示滤波器初始化标志
float display_filtered_temperature = 0;                             // 滤波后的显示温度

// 温度显示动画系统
static float displayed_temperature = 0;                    // 当前屏幕显示的温度值
static uint32_t last_display_update = 0;                  // 上次显示更新时间
static uint8_t first_display_update = 1;                  // 首次显示更新标志

// 系统状态变量声明
extern uint16_t DMA_ADC[3];           // DMA传输的ADC原始数据（通道0:温度，通道1:电压）
extern uint8_t heating_control_enabled; // PID控制使能标志

// 系统安全参数
#define USB_VOLTAGE_THRESHOLD 15.0f  // USB电压最低工作阈值（低于此值停止加热）

// 前向声明
void Level1Item3Action(void);
void Level1Item3Display(void);
uint8_t is_static_display_mode(void);
void exit_static_display_mode(void);
void Toggle9PointCalibrationAction(void);

// 显示系统函数声明
float smoothTemperatureDisplay(float current_display, float target_temp, float time_delta);



// 示例子菜单项
MenuItem subMenu1Items[] = {
    {"Sub1-Item1", Menu_DefaultAction, NULL, 0},
    {"Sub1-Item2", Menu_DefaultAction, NULL, 0},
    {"Sub1-Item3", Menu_DefaultAction, NULL, 0},
    {NULL, NULL, NULL, 0}  // 结束标记
};

MenuItem subMenu2Items[] = {
    {"Sub2-Item1", Menu_DefaultAction, NULL, 0},
    {"Sub2-Item2", Menu_DefaultAction, NULL, 0},
    {NULL, NULL, NULL, 0}  // 结束标记
};

MenuItem subMenu3Items[] = {
    {"Temperature Calib", SubMenu3CalibrationAction, NULL, 0},
    {"Sub3-Item2", Menu_DefaultAction, NULL, 0},
    {"Sub3-Item3", Menu_DefaultAction, NULL, 0},
    {"Sub3-Item4", Menu_DefaultAction, NULL, 0},
    {NULL, NULL, NULL, 0}  // 结束标记
};

// 二级菜单项
MenuItem level2MenuItems[] = {
    {"SubMenu1", NULL, subMenu1Items, 3},  // 3个有效菜单项
    {"SubMenu2", NULL, subMenu2Items, 2},  // 2个有效菜单项
    {"Temperature Calib", SubMenu3CalibrationAction, NULL, 0},
    {"9-Point Calib", Toggle9PointCalibrationAction, NULL, 0},
    {NULL, NULL, NULL, 0}  // 结束标记
};

// 一级菜单项
MenuItem rootMenuItems[] = {
    {"Level1-Item1", Menu_DefaultAction, NULL, 0},
    {"Level1-Item2", Menu_DefaultAction, NULL, 0},
    {"Level2Menu", NULL, level2MenuItems, 4},
    {"6-Axis Calibration", Level1Item3Action, NULL, 0},
    {NULL, NULL, NULL, 0}  // 结束标记
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

/**
 * @brief 9点校准开关切换动作函数
 */
void Toggle9PointCalibrationAction(void) {
    // 获取当前计算方式
    uint8_t current_method = getCurrentVoltageCalculationMethod();
    
    // 切换计算方式（0=线性计算，1=9点插值）
    uint8_t new_method = (current_method == 0) ? 1 : 0;
    
    // 设置新的计算方式
    selectVoltageCalculationMethod(new_method);
    
    // 可以添加一些视觉反馈，比如LED闪烁或屏幕提示
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
 * @brief 获取当前菜单项数量
 * @param ctx 菜单上下文指针
 * @return 菜单项数量
 */
static uint8_t getCurrentMenuItemCount(MenuContext* ctx) {
    uint8_t itemCount = 0;
    
    // 获取当前菜单项数量（统一使用遍历方式）
    while (itemCount < MAX_MENU_ITEMS && ctx->currentMenu[itemCount].name != NULL) {
        itemCount++;
    }
    
    // 如果遍历失败（比如菜单数组没有以NULL结尾），使用后备方案
    if (itemCount == 0) {
        if (ctx->menuLevel == 0) {
            itemCount = sizeof(rootMenuItems) / sizeof(MenuItem) - 1; // 减去结束标记
        } else {
            itemCount = ctx->menuStack[ctx->menuLevel]->subMenuCount;
        }
    }
    
    // 确保不超过最大菜单项数
    if (itemCount > MAX_MENU_ITEMS) {
        itemCount = MAX_MENU_ITEMS;
    }
    
    return itemCount;
}

/**
 * @brief 处理菜单输入
 * @param ctx 菜单上下文指针
 * @param direction 输入方向
 */
void Menu_HandleInput(MenuContext* ctx, MenuDirection direction) {
    uint8_t itemCount = getCurrentMenuItemCount(ctx);
    
    switch (direction) {
        case MENU_DIRECTION_UP:
            if (ctx->currentItemIndex > 0) {
                ctx->currentItemIndex--;
            } else {
                // 光标在第一项时向上移动，跳转到最后一项
                ctx->currentItemIndex = (itemCount > 0) ? itemCount - 1 : 0;
            }
            break;
            
        case MENU_DIRECTION_DOWN:
            if (ctx->currentItemIndex < itemCount - 1 && itemCount > 0) {
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
 * @brief 显示单个菜单项
 * @param u8g2 u8g2显示对象指针
 * @param item 菜单项
 * @param y Y坐标
 * @param isSelected 是否选中
 */
static void displayMenuItem(u8g2_t* u8g2, MenuItem* item, uint8_t y, uint8_t isSelected) {
    // 如果是当前选中项，显示箭头
    if (isSelected) {
        u8g2_DrawStr(u8g2, 0, y, ">");
    }
    
    // 显示菜单项名称
    u8g2_DrawStr(u8g2, 10, y, item->name);
    
    // 特殊处理：显示9点校准开关状态
    if (strcmp(item->name, "9-Point Calib") == 0) {
        uint8_t current_method = getCurrentVoltageCalculationMethod();
        u8g2_DrawStr(u8g2, 90, y, current_method == 1 ? "[ON]" : "[OFF]");
    }
    
    // 如果有子菜单，显示标记
    if (item->subMenu != NULL) {
        u8g2_DrawStr(u8g2, 110, y, ">");
    }
}

/**
 * @brief 显示菜单
 * @param ctx 菜单上下文指针
 * @param u8g2 u8g2显示对象指针
 */
void Menu_Display(MenuContext* ctx, u8g2_t* u8g2) {
    uint8_t i;
    uint8_t itemCount = getCurrentMenuItemCount(ctx);
    uint8_t startY = 20;
    uint8_t lineHeight = 12;
    
    // 清除显示缓冲区
    u8g2_ClearBuffer(u8g2);
    
    // 显示标题
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 0, 10, "MENU:");
    
    // 显示菜单项（确保不超过有效菜单项数量）
    for (i = 0; i < itemCount && i < MAX_MENU_ITEMS; i++) {
        // 检查菜单项是否有效
        if (ctx->currentMenu[i].name == NULL) {
            break;
        }
        uint8_t y = startY + i * lineHeight;
        displayMenuItem(u8g2, &ctx->currentMenu[i], y, i == ctx->currentItemIndex);
    }
    
    // 显示层级信息
    char levelStr[20];
    sprintf(levelStr, "Level: %d", ctx->menuLevel + 1);
    u8g2_DrawStr(u8g2, 0, 80, levelStr);
    
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
 * @brief 处理静态显示模式下的按键
 * @param key 按键类型
 * @return 1表示已处理，0表示未处理
 */
static uint8_t handleStaticDisplayKey(KeyType key) {
    if (!is_static_display_mode()) {
        return 0;
    }
    
    switch (key) {
        case KEY_MODE:
            // 在静态显示模式下，KEY_MODE键执行归零校准
            St1xStatic_ManualZeroCalibration();
            buzzerConfirmBeep();
            return 1;
            
        case KEY_MODE_LONG:
            // 在静态显示模式下，长按KEY_MODE键用于退出
            exit_static_display_mode();
            return 1;
            
        case KEY_UP:
        case KEY_DOWN:
            // 在静态显示模式下，UP/DOWN键无操作
            return 1;
            
        default:
            return 0;
    }
}

/**
 * @brief 处理正常菜单模式下的按键
 * @param key 按键类型
 * @return 1表示菜单仍在运行，0表示菜单已退出
 */
static uint8_t handleNormalMenuKey(KeyType key) {
    switch (key) {
        case KEY_UP:
            Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_DOWN);
            break;
            
        case KEY_DOWN:
            Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_UP);
            break;
            
        case KEY_MODE:
            Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_ENTER);
            break;
            
        case KEY_MODE_LONG:
            if (g_menuCtx.menuLevel > 0) {
                Menu_HandleInput(&g_menuCtx, MENU_DIRECTION_BACK);
            } else {
                menu_active = 0;
                return 0;
            }
            break;
            
        default:
            break;
    }
    return 1;
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
    if (!CalibrationSystem_IsActive() && (HAL_GetTick() - last_operation_time) >= AUTO_RETURN_TIME) {
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
    if (CalibrationSystem_IsActive()) {
        // 在校准模式下，按键由校准模块处理
        CalibrationSystem_HandleKey(key);
    } else {
        // 先处理静态显示模式按键
        if (!handleStaticDisplayKey(key)) {
            // 如果不是静态显示模式，处理正常菜单按键
            if (!handleNormalMenuKey(key)) {
                return 0;
            }
        }
    }
    
    // 获取U8G2对象
    extern u8g2_t u8g2;
    
    // 检查是否处于校准模式
    if (CalibrationSystem_IsActive()) {
        // 在校准模式下，显示校准界面
        CalibrationSystem_Update(&u8g2);
    } else if (is_static_display_mode()) {
        // 处理静态数据显示
        Level1Item3Display();
    } else {
        // 在菜单模式下，始终显示菜单界面（包括根菜单级别）
        Menu_Display(&g_menuCtx, &u8g2);
    }
    
    return 1; // 菜单仍在运行
}

// ==================== 显示系统函数实现 ====================

/**
 * @brief 温度传感器参数配置
 */
#define ATemp   0              // 环境温度补偿值（冷端补偿）
#define Thermal_Voltage 0.0033f // 热电偶电压-温度转换系数（mV/°C）



/**
 * @brief 更新显示用温度滤波器
 * 
 * 功能：为OLED显示提供平滑的温度读数
 * 原理：使用8点移动平均滤波，提供稳定的显示效果
 * 特点：大窗口滤波，响应平滑，适合视觉显示
 * 
 * @param adcValue ADC原始读数
 */
void updateDisplayTemperatureFilter(uint16_t adcValue) {
    // 防止传感器读数异常
    if (adcValue > 4000) {
        return;
    }
    
    float current_temp = calculateT12Temperature(adcValue);
    
    // 如果是第一次使用，初始化缓冲区和显示温度
    if (!display_filter_initialized) {
        for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
            display_temperature_buffer[i] = current_temp;
        }
        display_filtered_temperature = current_temp;
        
        // 只在重启后首次初始化时设置显示温度，但保持first_display_update标志不变
        if (first_display_update) {
            displayed_temperature = current_temp;  // 初始化显示温度
        }
        
        display_filter_initialized = 1;
    }
    
    display_temperature_buffer[display_filter_index] = current_temp;
    display_filter_index = (display_filter_index + 1) % DISPLAY_FILTER_SIZE;
    
    // 计算平均值
    float sum = 0;
    for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
        sum += display_temperature_buffer[i];
    }
    display_filtered_temperature = sum / DISPLAY_FILTER_SIZE;
}

// 显示用滤波温度变量声明（直接使用变量替代函数）
extern float display_filtered_temperature;

/**
 * @brief 温度显示平滑动画函数
 * 
 * 功能：实现无停顿的连续温度显示动画效果
 * 原理：基于动态速度因子的智能插值算法
 * 特点：温差越大速度越快，温差越小越精确
 * 速度控制：适应8-12秒快速升温需求
 * 安全限制：限制最大单次变化幅度

/**
 * @brief 平滑温度显示算法
 * 功能：实现温度显示的平滑动画效果，在停止加热或不在工作状态时立即响应实际温度变化
 * 
 * @param current_display 当前显示温度
 * @param target_temp 目标温度（实际传感器温度）
 * @param time_delta 时间差（秒）
 * @return float 新的显示温度
 */
float smoothTemperatureDisplay(float current_display, float target_temp, float time_delta) {
    // 检查加热状态：如果停止加热或不在工作状态，立即响应实际温度
    extern uint8_t heating_control_enabled;
    extern uint8_t heating_status;
    
    // 如果PID未工作或加热已停止，直接显示实际温度
    if (!heating_control_enabled || !heating_status) {
        return target_temp;
    }
    
    // 正常加热状态下的平滑显示算法
    float temp_diff = target_temp - current_display;
    float abs_temp_diff = fabs(temp_diff);
    
    // 动态速度因子：大幅提升响应速度，实现7秒内显示
    float speed_factor = 0.0f;
    
    // 优化速度控制：适应7秒内快速显示
    if (abs_temp_diff > 100.0f) {
        speed_factor = 120.0f; // 超大温差极速响应
    } else if (abs_temp_diff > 50.0f) {
        speed_factor = 80.0f;  // 大温差快速响应
    } else if (abs_temp_diff > 30.0f) {
        speed_factor = 60.0f;  // 中等温差快速响应
    } else if (abs_temp_diff > 15.0f) {
        speed_factor = 40.0f;  // 小温差快速响应
    } else if (abs_temp_diff > 8.0f) {
        speed_factor = 25.0f;  // 接近目标快速响应
    } else if (abs_temp_diff > 3.0f) {
        speed_factor = 15.0f;  // 精确接近阶段
    } else if (abs_temp_diff > 2.0f) {
        speed_factor = 6.0f;   // PID稳定区边缘：更平稳
    } else if (abs_temp_diff > 1.0f) {
        speed_factor = 3.0f;   // PID稳定区：非常平稳
    } else {
        speed_factor = 1.5f;   // 最终稳定阶段：极平稳
    }
    
    // 根据温度阶段智能加速
    if (current_display < 150.0f) {
        speed_factor *= 1.8f; // 低温阶段大幅加速
    } else if (current_display > 300.0f) {
        speed_factor *= 0.9f; // 高温阶段轻微减速
    }
    
    // 计算目标步长（度/秒 * 时间差）
    float target_step = speed_factor * time_delta;
    
    // 确保步长方向正确
    if (temp_diff < 0) {
        target_step = -target_step;
    }
    
    // 放宽最大单次变化幅度限制
    float max_step = 15.0f; // 放宽限制以支持更快响应
    if (fabs(target_step) > max_step) {
        target_step = (target_step > 0) ? max_step : -max_step;
    }
    
    // 确保显示温度不会超过实际温度
    if (fabs(temp_diff) < fabs(target_step)) {
        // 如果步长会超过目标温度，直接到达目标
        return target_temp;
    } else {
        // 否则正常更新
        float new_display = current_display + target_step;
        
        // 强制最小变化，确保显示持续更新
        if (abs_temp_diff > 0.5f && fabs(target_step) < 0.5f) {
            new_display += (temp_diff > 0) ? 0.5f : -0.5f;
        }
        
        return new_display;
    }
}

/**
 * @brief 主显示界面绘制函数
 * 
 * 功能：在OLED屏幕上实时显示温度、电压、状态等信息
 * 显示内容：当前温度、目标温度、PID温度、ADC值、USB电压、加热状态
 * 重启检测：使用系统启动时间和PID状态双重判断重启状态
 * 温度显示：重启后直接显示实际温度，正常启动使用平滑动画
 * 
 * @param u8g2 OLED显示对象指针
 */
void drawMainDisplay(u8g2_t *u8g2) {
    char display_buffer[32];
    
    // 检查静置状态，如果是静置状态则不更新显示内容
    extern uint8_t St1xStatic_IsInStandbyMode(void);
    
    uint8_t in_standby_mode = St1xStatic_IsInStandbyMode();
    
    // 如果是静置状态且正在加热（降温状态），则只更新屏幕亮度，不跳过显示内容
    // 修改：移除直接return的逻辑，确保屏幕在静置状态下仍然刷新
    
    // 确保传感器数据正在更新
    extern uint8_t adc_sampling_flag;
    if (adc_sampling_flag == 0) {
        extern ADC_HandleTypeDef hadc1;
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 3);
    }

    // 获取要显示的数据
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.151f;

    // 直接使用传感器数据计算显示温度
    float raw_temp = calculateT12Temperature(DMA_ADC[0]);

    // 更新显示用滤波器
    updateDisplayTemperatureFilter(DMA_ADC[0]);
    // 使用滤波后的显示温度（直接使用变量）
    float filtered_temp = display_filtered_temperature;
    
    // 获取控制用的滤波温度（直接使用变量）
    extern float filtered_temperature;
    float pid_temp = filtered_temperature;
    
    // 实现平滑的温度显示动画效果
    uint32_t current_time = HAL_GetTick();
    
    // 温度达到目标温度检测（误差-10度）
    static uint8_t temperature_reached_flag = 0;
    float temperature_error = filtered_temp - target_temperature;
    
    // 如果温度达到目标温度误差-10度以内，且正在加热，播放提示音
    if (heating_status && heating_control_enabled && temperature_error >= -10.0f && temperature_error <= 0.0f) {
        if (!temperature_reached_flag) {
            buzzerConfirmBeep();  // 播放确认音
            temperature_reached_flag = 1;
        }
    } else {
        // 重置标志
        temperature_reached_flag = 0;
    }
    
    // 强制高频更新显示，消除停顿感
    // 每次调用都更新显示，确保流畅性
    
    // 判断是否重启后首次显示：使用系统启动时间和PID状态双重判断
    static uint32_t system_start_time = 0;
    if (system_start_time == 0) {
        system_start_time = current_time;  // 记录系统启动时间
    }
    
    // 重启检测：系统启动时间较短（<2秒）且PID未工作
    if ((current_time - system_start_time) < 2000 && !heating_control_enabled) {
        // 重启后首次显示更新，直接使用当前实际温度，不使用平滑算法
        last_display_update = current_time;
        displayed_temperature = filtered_temp;
    } else if (first_display_update) {
        // 正常启动后的首次显示，使用平滑算法
        float time_delta = (current_time - last_display_update) / 1000.0f;
        last_display_update = current_time;
        
        if (time_delta > 0.05f) time_delta = 0.02f;
        
        displayed_temperature = smoothTemperatureDisplay(displayed_temperature, filtered_temp, time_delta);
        first_display_update = 0;  // 清除首次显示标志
    } else {
        // 正常显示更新，使用平滑算法
        // 计算时间差（秒），大幅提高更新频率
        float time_delta = (current_time - last_display_update) / 1000.0f;
        last_display_update = current_time;
        
        // 大幅提高更新频率，适应快速升温
        if (time_delta > 0.05f) time_delta = 0.02f; // 降低上限到20ms，确保高频更新
        
        // 调用平滑显示函数
        displayed_temperature = smoothTemperatureDisplay(displayed_temperature, filtered_temp, time_delta);
        
        // 借鉴Arduino项目的稳定显示策略：在±1°C范围内直接显示设定点温度
        if (fabs(displayed_temperature - target_temperature) <= 1.0f) {
            displayed_temperature = target_temperature;
        }
    }
    
    // 完整清除屏幕缓冲区，避免画面残留
    u8g2_ClearBuffer(u8g2);
    
    // 显示当前温度（大字体）
    u8g2_SetFont(u8g2, u8g2_font_fur30_tf);
    sprintf(display_buffer, "%0.0f", displayed_temperature);
    u8g2_DrawStr(u8g2, 33, 56, display_buffer);
    u8g2_DrawStr(u8g2, 3, 56, "C");
    
    // 显示其他信息（小字体）
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    
    // 显示控制用温度
    sprintf(display_buffer, "PID:%0.0f", pid_temp);
    u8g2_DrawStr(u8g2, 3, 80, display_buffer);
    
    // 显示传感器原始数据
    sprintf(display_buffer, "ADC0:%d", DMA_ADC[0]);
    u8g2_DrawStr(u8g2, 4, 11, display_buffer);
    
    // 显示USB电压
    sprintf(display_buffer, "USB:%0.1f V", usb_voltage);
    u8g2_DrawStr(u8g2, 67, 11, display_buffer);
    
    // 显示电压状态
    if (usb_voltage >= USB_VOLTAGE_THRESHOLD) {
        u8g2_DrawStr(u8g2, 114, 24, "OK");
    } else {
        u8g2_DrawStr(u8g2, 108, 24, "LOW");
    }
    
    // 显示环境温度（基于烙铁笔项目的思路）
    extern float ambient_temperature;  // 直接使用环境温度变量
    float ambient_temp = ambient_temperature;
    sprintf(display_buffer, "Amb:%0.0f", ambient_temp);
    u8g2_DrawStr(u8g2, 86, 71, display_buffer);
    
    // 显示目标温度
    sprintf(display_buffer, "SET:%0.0f", target_temperature);
    u8g2_DrawStr(u8g2, 80, 80, display_buffer);
    
    // 显示加热状态
    // 更友好的状态显示逻辑
    if (!heating_control_enabled) {
        // 1. PID是否工作-否-不显示
        // 不显示任何状态文字
    } else if (!heating_status) {
        // 2. PID是否工作-是-加热状态-否-显示"Stop"
        u8g2_DrawStr(u8g2, 1, 68, "Stop");
    } else if (focused_heating_mode) {
        // 3. PID是否工作-是-是否进入专注模式-是-显示"Heating"
        u8g2_DrawStr(u8g2, 1, 68, "Heating");
    } else {
        // 4. PID是否工作-是-是否进入专注模式-否-PID是否在控制状态-是-显示"Work"
        u8g2_DrawStr(u8g2, 1, 68, "Work");
    }

    // 把缓冲区内容发送到屏幕显示
    u8g2_SendBuffer(u8g2);
}