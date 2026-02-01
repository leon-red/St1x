#include "St1xSystemManager.h"
#include "St1xFocusedHeating.h"
#include "u8g2_oled.h"
#include "St1xKey.h"
#include "St1xMenu.h"
#include "St1xStatic.h"
#include "St1xCalibrationSystem.h"
#include "Buzzer.h"
#include <math.h>

// 外部变量声明
extern u8g2_t u8g2;
extern uint8_t menu_active;
extern uint8_t debug_display_enabled;
extern uint8_t timer_started;
extern uint32_t last_oled_update;
extern uint32_t last_standby_check;
extern float last_displayed_temp;
extern uint32_t menu_enter_delay;

// 系统运行模式
static SystemMode current_system_mode = SYSTEM_MODE_NORMAL;

// 系统管理初始化函数
void SystemManager_Init(void) {
    // 初始化系统状态变量
    menu_active = 0;
    last_oled_update = 0;
    debug_display_enabled = 0;
    current_system_mode = SYSTEM_MODE_NORMAL;
    timer_started = 0;
    last_standby_check = 0;
    last_displayed_temp = 0;
    menu_enter_delay = 0;
    
    // 初始化专注加热状态机
    FocusedHeating_Init();
}

// 系统模式管理函数
void SystemManager_EnterMenuMode(void) {
    current_system_mode = SYSTEM_MODE_MENU;
    menu_active = 1;
    Menu_InitSystem();  // 初始化菜单系统
}

void SystemManager_EnterCalibrationMode(void) {
    current_system_mode = SYSTEM_MODE_CALIBRATION;
    CalibrationSystem_Start();
}

void SystemManager_ExitCalibrationMode(void) {
    CalibrationSystem_Stop();
    current_system_mode = SYSTEM_MODE_NORMAL;
}

// 系统状态处理函数
void SystemManager_ModeHandler(uint32_t current_time) {
    switch (current_system_mode) {
        case SYSTEM_MODE_NORMAL:
            SystemManager_NormalModeHandler(current_time);
            break;
            
        case SYSTEM_MODE_MENU:
            SystemManager_MenuModeHandler();
            break;
            
        case SYSTEM_MODE_CALIBRATION:
            SystemManager_CalibrationModeHandler(current_time);
            break;
    }
}

void SystemManager_NormalModeHandler(uint32_t current_time) {
    // 检查校准系统是否激活，如果激活则移交控制权
    if (CalibrationSystem_IsActive()) {
        // 校准系统激活时，将系统模式切换到校准模式
        current_system_mode = SYSTEM_MODE_CALIBRATION;
        return;
    }
    
    KeyType key = Key_Scan();
    static uint8_t key_debug_display = 0;
    static uint32_t key_debug_time = 0;
    
    // 处理按键输入
    if (key != KEY_NONE) {
        // 处理温度调节按键
        if (key == KEY_UP || key == KEY_DOWN) {
            handleMainTemperatureAdjust(key);
        }
        
        // 处理MODE键短按 - 加热控制或调试模式下的归零校准
        if (key == KEY_MODE) {
            if (debug_display_enabled) {
                // 调试显示模式下，执行归零校准
                St1xStatic_ManualZeroCalibration();
                buzzerConfirmBeep();  // 播放确认音
            } else {
                // 正常模式下，执行加热控制
                handleHeatingControl();
            }
        }
        
        // 显示按键状态（用于调试）
        key_debug_display = 1;
        key_debug_time = current_time;
        
        // 处理菜单键 - 如果检测到长按，进入菜单模式
        if (key == KEY_MODE_LONG) {
            menu_enter_delay = current_time + 200;  // MENU_ENTER_DELAY
        }
    }
    
    // 延迟进入菜单模式
    if (menu_enter_delay > 0 && current_time >= menu_enter_delay) {
        menu_enter_delay = 0;
        SystemManager_EnterMenuMode();
    }
    
    // 检测需要更新显示的条件
    uint8_t need_display_update = 0;
    
    // 有按键按下时，需要更新显示
    if (key != KEY_NONE) {
        need_display_update = 1;
    }
    
    // 温度变化超过阈值时也需要更新显示
    extern float display_filtered_temperature;  // 直接使用显示滤波温度变量
    float current_temp = display_filtered_temperature;
    if (fabs(current_temp - last_displayed_temp) > 0.5f) {  // TEMP_DISPLAY_THRESHOLD
        need_display_update = 1;
        last_displayed_temp = current_temp;
    }
    
    // 定期更新显示
    if ((current_time - last_oled_update) >= 100) {  // DISPLAY_UPDATE_INTERVAL
        need_display_update = 1;
    }
    
    // 如果需要更新显示，则执行显示操作
    if (need_display_update) {
        drawMainDisplay(&u8g2);
        
        // 如果启用了调试显示，则显示调试信息
        if (debug_display_enabled) {
            St1xStatic_DisplayDebugInfo(&u8g2);
        }
        
        u8g2_SendBuffer(&u8g2);
        last_oled_update = current_time;
        
        // 首次显示更新完成后，启动TIM2中断（PID控制）
        if (!timer_started) {
            extern TIM_HandleTypeDef htim2;
            HAL_TIM_Base_Start_IT(&htim2);  // 启动TIM2中断
            timer_started = 1;
        }
    }
}

void SystemManager_MenuModeHandler(void) {
    if (!Menu_Process()) {
        // 菜单已退出，返回正常模式
        current_system_mode = SYSTEM_MODE_NORMAL;
        menu_active = 0;
    }
}

void SystemManager_CalibrationModeHandler(uint32_t current_time) {
    KeyType key = Key_Scan();
    
    // 处理校准模式下的按键
    if (key != KEY_NONE) {
        CalibrationSystem_HandleKey(key);
    }
    
    CalibrationSystem_Update(&u8g2);
}



// 应用模块初始化
void SystemManager_AppModulesInit(void) {
    Key_Init();                     // 初始化按键处理
    St1xStatic_Init();             // 初始化静态传感器显示
    CalibrationSystem_Init();      // 初始化独立校准系统
    St1xStatic_SetDefaultStandbyParameters();  // 设置默认参数
    
    // 初始化冷端补偿温度（使用刚上电时的环境温度）
    extern void initializeColdJunctionTemperature(void);
    initializeColdJunctionTemperature();
    
    // 初始化冷启动状态
    extern void initializeColdStartState(void);
    initializeColdStartState();
}