/**
 * @file St1xCalibrationHardware.h
 * @brief 独立校准系统硬件抽象层头文件
 * 
 * 为独立校准系统提供硬件抽象层接口定义和函数声明
 */

#ifndef ST1X_CALIBRATION_HARDWARE_H
#define ST1X_CALIBRATION_HARDWARE_H

#include "St1xCalibrationSystem.h"

/**
 * @brief 获取校准系统硬件接口
 * 
 * @return const CalibrationHardwareInterface* 指向硬件接口的指针
 */
const CalibrationHardwareInterface* GetCalibrationHardwareInterface(void);

/**
 * @brief 校准系统专用按键扫描函数
 * 
 * 将主系统的按键类型转换为校准系统的按键类型
 * 
 * @return CalibrationKeyType 校准系统按键类型
 */
CalibrationKeyType CalibrationKeyScan(void);

/**
 * @brief 启动校准系统专用加热控制
 */
void StartCalibrationHeating(void);

/**
 * @brief 停止校准系统专用加热控制
 */
void StopCalibrationHeating(void);

/**
 * @brief 初始化校准系统硬件抽象层
 * 
 * 这个函数应该在主系统初始化时调用，用于设置校准系统的硬件接口
 */
void CalibrationHardware_Init(void);

/**
 * @brief 同步主系统的PID参数到校准系统
 * 
 * 这个函数用于确保校准系统使用与主系统完全相同的PID参数
 * 当主系统PID参数调整时，调用此函数即可自动同步
 */
void SyncPIDParametersFromMainSystem(void);

#endif /* ST1X_CALIBRATION_HARDWARE_H */