#ifndef ST1XCALIBRATION_SYSTEM_H
#define ST1XCALIBRATION_SYSTEM_H

#include "u8g2.h"
#include "St1xKey.h"
#include <stdint.h>

// 按键类型定义（使用主系统的按键类型）
typedef KeyType CalibrationKeyType;

// 校准系统状态
typedef enum {
    CAL_STATE_IDLE,         // 空闲状态
    CAL_STATE_VOLTAGE_CHECK, // 电压检测状态
    CAL_STATE_RUNNING,      // 校准运行中
    CAL_STATE_COMPLETE      // 校准完成
} CalibrationSystemState;

// 校准点状态
typedef enum {
    CAL_POINT_WAITING,      // 等待进入该点
    CAL_POINT_HEATING,      // 加热到目标温度
    CAL_POINT_STABLE,       // 温度已稳定
    CAL_POINT_ADJUSTED      // 已调整完成
} CalibrationPointState;

// 核心校准系统接口
void CalibrationSystem_Init(void);
void CalibrationSystem_Start(void);
void CalibrationSystem_Stop(void);
void CalibrationSystem_Update(u8g2_t *u8g2);
void CalibrationSystem_HandleKey(CalibrationKeyType key);
uint8_t CalibrationSystem_IsActive(void);

// 外部函数声明（需要在主系统中实现）
float getCalibrationTemperature(void);
uint8_t isUSBVoltageSufficient(void);
void StopCalibrationHeating(void);
void StartCalibrationHeating(void);

// PID控制器函数声明
void setT12Temperature(float temperature);

#endif // ST1XCALIBRATION_SYSTEM_H