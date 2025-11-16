#ifndef ST1XCALIBRATION_SYSTEM_H
#define ST1XCALIBRATION_SYSTEM_H

#include "u8g2.h"
#include "St1xKey.h"
#include <stdint.h>

// 按键类型定义
typedef KeyType CalibrationKeyType;

// 校准系统状态
typedef enum {
    CAL_STATE_IDLE,
    CAL_STATE_VOLTAGE_CHECK,
    CAL_STATE_RUNNING,
    CAL_STATE_COMPLETE
} CalibrationSystemState;

// 校准点状态
typedef enum {
    CAL_POINT_WAITING,
    CAL_POINT_HEATING,
    CAL_POINT_STABLE,
    CAL_POINT_ADJUSTED
} CalibrationPointState;

// 核心校准系统接口
void CalibrationSystem_Init(void);
void CalibrationSystem_Start(void);
void CalibrationSystem_Stop(void);
void CalibrationSystem_Update(u8g2_t *u8g2);
void CalibrationSystem_HandleKey(CalibrationKeyType key);
uint8_t CalibrationSystem_IsActive(void);

// 外部函数声明
uint8_t isUSBVoltageSufficient(void);

// PID控制器函数声明
void setT12Temperature(float temperature);

#endif // ST1XCALIBRATION_SYSTEM_H