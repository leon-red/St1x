#ifndef ST1XSYSTEMMANAGER_H_
#define ST1XSYSTEMMANAGER_H_

#include "main.h"

// 前向声明u8g2_t结构体
struct u8g2_struct;
typedef struct u8g2_struct u8g2_t;

// 系统运行模式枚举
typedef enum {
    SYSTEM_MODE_NORMAL,     // 正常运行模式
    SYSTEM_MODE_MENU,       // 菜单模式
    SYSTEM_MODE_CALIBRATION // 校准模式
} SystemMode;

// 系统管理初始化函数
void SystemManager_Init(void);

// 系统模式管理函数
void SystemManager_EnterMenuMode(void);
void SystemManager_EnterCalibrationMode(void);
void SystemManager_ExitCalibrationMode(void);

// 系统状态处理函数
void SystemManager_ModeHandler(uint32_t current_time);
void SystemManager_NormalModeHandler(uint32_t current_time);
void SystemManager_MenuModeHandler(void);
void SystemManager_CalibrationModeHandler(uint32_t current_time);

// 应用模块初始化
void SystemManager_AppModulesInit(void);

#endif /* ST1XSYSTEMMANAGER_H_ */