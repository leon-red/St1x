#ifndef ST1XCALIBRATION_SYSTEM_H
#define ST1XCALIBRATION_SYSTEM_H

#include "u8g2.h"
#include "St1xKey.h"
#include <stdint.h>

// 使用主系统的按键类型
typedef KeyType CalibrationKeyType;

// 核心校准系统接口
void CalibrationSystem_Init(void);
void CalibrationSystem_Start(void);
void CalibrationSystem_Stop(void);
void CalibrationSystem_Update(u8g2_t *u8g2);
void CalibrationSystem_HandleKey(CalibrationKeyType key);
uint8_t CalibrationSystem_IsActive(void);
void CalibrationSystem_SaveData(void);

// 外部函数声明（需要在主系统中实现）
float getCalibrationTemperature(void);
uint8_t isUSBVoltageSufficient(void);
void StopCalibrationHeating(void);
void StartCalibrationHeating(void);

// PID控制器函数声明
void setT12Temperature(float temperature);

#endif // ST1XCALIBRATION_SYSTEM_H