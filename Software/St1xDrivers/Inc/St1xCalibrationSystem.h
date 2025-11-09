#ifndef ST1XCALIBRATION_SYSTEM_H
#define ST1XCALIBRATION_SYSTEM_H

#include "u8g2.h"
#include <stdint.h>

// 硬件抽象层接口定义
typedef enum {
    CAL_KEY_NONE = 0,
    CAL_KEY_UP,
    CAL_KEY_DOWN,
    CAL_KEY_MODE,
    CAL_KEY_MODE_LONG
} CalibrationKeyType;

// 独立校准系统状态
typedef enum {
    CAL_STATE_IDLE,         // 空闲状态
    CAL_STATE_VOLTAGE_CHECK, // 电压检测状态
    CAL_STATE_RUNNING,      // 校准运行中
    CAL_STATE_COMPLETE,     // 校准完成
    CAL_STATE_ERROR         // 校准错误
} CalibrationSystemState;

// 校准点状态
typedef enum {
    CAL_POINT_WAITING,      // 等待进入该点
    CAL_POINT_HEATING,      // 加热到目标温度
    CAL_POINT_STABLE,       // 温度已稳定
    CAL_POINT_ADJUSTED      // 已调整完成
} CalibrationPointState;

// 硬件抽象层回调函数类型定义
typedef void (*CalibrationSetTemperatureFunc)(float temperature);
typedef float (*CalibrationGetTemperatureFunc)(void);
typedef void (*CalibrationStartHeatingFunc)(void);
typedef void (*CalibrationStopHeatingFunc)(void);
typedef CalibrationKeyType (*CalibrationKeyScanFunc)(void);
typedef uint8_t (*CalibrationVoltageCheckFunc)(void);
typedef uint32_t (*CalibrationGetTickFunc)(void);
typedef uint8_t (*CalibrationSaveDataFunc)(const float* offsets, uint8_t count);
typedef void (*CalibrationStartHeatingTimerFunc)(void);
typedef void (*CalibrationStopHeatingTimerFunc)(void);

// PID参数获取接口
typedef float (*CalibrationGetPIDParameterFunc)(void);
typedef uint8_t (*CalibrationGetHeatingStatusFunc)(void);
typedef uint8_t (*CalibrationGetFocusedHeatingModeFunc)(void);
typedef void (*CalibrationSetFocusedHeatingModeFunc)(uint8_t mode);

// 温度限制管理接口
typedef void (*CalibrationSetTemperatureLimitFunc)(float limit);
typedef float (*CalibrationGetTemperatureLimitFunc)(void);

// 温度传感器参数获取接口
typedef float (*CalibrationGetThermalVoltageFunc)(void);
typedef float (*CalibrationGetColdJunctionTempFunc)(void);

// 系统配置参数获取接口（仅同步需要与主系统一致的参数）
typedef uint32_t (*CalibrationGetKeyDebounceTimeFunc)(void);
typedef uint32_t (*CalibrationGetKeyLongPressTimeFunc)(void);

// 硬件抽象层配置结构体
typedef struct {
    CalibrationSetTemperatureFunc setTemperature;
    CalibrationGetTemperatureFunc getTemperature;
    CalibrationStartHeatingFunc startHeating;
    CalibrationStopHeatingFunc stopHeating;
    CalibrationKeyScanFunc keyScan;
    CalibrationVoltageCheckFunc voltageCheck;
    CalibrationGetTickFunc getTick;
    CalibrationSaveDataFunc saveData;
    CalibrationStartHeatingTimerFunc startHeatingTimer;
    CalibrationStopHeatingTimerFunc stopHeatingTimer;
    
    // PID参数获取接口
    CalibrationGetPIDParameterFunc getPID_Kp;
    CalibrationGetPIDParameterFunc getPID_Ki;
    CalibrationGetPIDParameterFunc getPID_Kd;
    CalibrationGetHeatingStatusFunc getHeatingStatus;
    CalibrationGetFocusedHeatingModeFunc getFocusedHeatingMode;
    CalibrationSetFocusedHeatingModeFunc setFocusedHeatingMode;
    
    // 温度限制管理接口
    CalibrationSetTemperatureLimitFunc setTemperatureLimit;
    CalibrationGetTemperatureLimitFunc getTemperatureLimit;
    
    // 温度传感器参数获取接口
    CalibrationGetThermalVoltageFunc getThermalVoltage;
    CalibrationGetColdJunctionTempFunc getColdJunctionTemp;
    
    // 系统配置参数获取接口（仅同步需要与主系统一致的参数）
    CalibrationGetKeyDebounceTimeFunc getKeyDebounceTime;
    CalibrationGetKeyLongPressTimeFunc getKeyLongPressTime;
} CalibrationHardwareInterface;

// 独立校准系统接口
void CalibrationSystem_Init(const CalibrationHardwareInterface* hw_interface);
void CalibrationSystem_Start(void);
void CalibrationSystem_Stop(void);
void CalibrationSystem_Update(u8g2_t *u8g2);
void CalibrationSystem_HandleKey(CalibrationKeyType key);
uint8_t CalibrationSystem_IsActive(void);
CalibrationSystemState CalibrationSystem_GetState(void);

// 温度限制管理接口（支持临时变更）
float CalibrationSystem_GetTemperatureLimit(void);
void CalibrationSystem_SetTemperatureLimit(float limit);

// 兼容原有校准系统的接口（保持向后兼容）
void St1xCalibration_Exit(void);
void St1xCalibration_HandleKey(CalibrationKeyType key);
void St1xCalibration_MainLoop(u8g2_t *u8g2);
uint8_t St1xCalibration_IsInProgress(void);
void St1xCalibration_ClearData(void);

#endif // ST1XCALIBRATION_SYSTEM_H