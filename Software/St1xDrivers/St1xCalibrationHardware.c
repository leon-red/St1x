/**
 * @file St1xCalibrationHardware.c
 * @brief 独立校准系统硬件抽象层实现
 * 
 * 为独立校准系统提供具体的硬件接口实现，实现硬件抽象层接口
 */

#include "St1xCalibrationSystem.h"
#include "St1xADC.h"
#include "St1xPID.h"
#include "St1xFlash.h"
#include "St1xKey.h"
#include "tim.h"
#include "main.h"
#include <stdio.h>

// 外部变量声明
extern PID_Controller t12_pid;
extern uint8_t heating_status;
extern uint8_t focused_heating_mode;  // 专注加热模式标志

/**
 * @brief 校准系统专用温度获取函数（绕过460度限制）
 * 
 * 这个函数用于校准系统，返回原始温度值，不进行460度限制
 * 
 * @return 原始温度值（摄氏度）
 */
float getCalibrationTemperature(void) {
    // 外部变量声明 - 从St1xADC.c中获取ADC数据
    extern uint16_t DMA_ADC[2];  // ADC数据数组，[0]为温度传感器，[1]为USB电压
    
    // 直接调用ADC获取原始温度值，不进行460度限制
    uint16_t adcValue = DMA_ADC[0];  // 获取ADC原始值（温度传感器）
    
    // 使用与主系统相同的传感器参数，确保温度计算一致性
    const float mV_per_degree = 0.00263f;  // 每摄氏度对应的电压
    const float cold_junction_temp = 0;     // 环境温度补偿
    const float adc_ref_voltage = 3.3;      // ADC参考电压
    const uint16_t adc_max = 4095;          // ADC最大值
    
    // 防止读数异常
    if (adcValue > adc_max) adcValue = adc_max;
    
    // 数字信号转电压值
    float voltage = (adcValue * adc_ref_voltage) / adc_max;
    
    // 电压值转温度值（不进行460度限制）
    float temperature = voltage / mV_per_degree + cold_junction_temp;
    
    // 只进行下限保护，不限制上限（这是关键修改）
    if (temperature < 0) temperature = 0;
    
    // 调试输出：检查实际获取的温度值
    printf("[CalibrationTemp] ADC=%d, Voltage=%.4fV, Temp=%.1f°C\n", 
           adcValue, voltage, temperature);
    
    return temperature;
}

// 硬件接口实现实例
static const CalibrationHardwareInterface calibration_hw_interface = {
    .getTick = HAL_GetTick,
    .getTemperature = getCalibrationTemperature,
    .setTemperature = setTemperatureForCalibration,
    .voltageCheck = isUSBVoltageSufficient,
    .keyScan = CalibrationKeyScan,
    .startHeating = StartCalibrationHeating,
    .stopHeating = StopCalibrationHeating,
    .saveData = St1xFlash_SaveCalibrationData,
    .startHeatingTimer = startHeatingControlTimer,
    .stopHeatingTimer = stopHeatingControlTimer
};

/**
 * @brief 获取校准系统硬件接口
 */
const CalibrationHardwareInterface* GetCalibrationHardwareInterface(void) {
    return &calibration_hw_interface;
}

/**
 * @brief 校准系统专用按键扫描函数
 * 
 * 将主系统的按键类型转换为校准系统的按键类型
 */
CalibrationKeyType CalibrationKeyScan(void) {
    KeyType key = Key_Scan();
    
    switch (key) {
        case KEY_UP:
            return CAL_KEY_UP;
        case KEY_DOWN:
            return CAL_KEY_DOWN;
        case KEY_MODE:
            return CAL_KEY_MODE;
        case KEY_NONE:
        default:
            return CAL_KEY_NONE;
    }
}

/**
 * @brief 启动校准系统专用加热控制
 */
void StartCalibrationHeating(void) {
    if (!heating_status) {
        heating_status = 1;
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);  // 启动PWM输出
        startHeatingControlTimer();
    }
}

/**
 * @brief 校准系统专用温度设置函数
 * 
 * 这个函数用于校准系统，设置目标温度并重置PID状态
 * 特别针对高温校准进行了优化
 * 
 * @param temperature 目标温度
 */
void setTemperatureForCalibration(float temperature) {
    printf("[CalibrationTemp] Setting calibration temperature to %.1f°C\n", temperature);
    
    // 重置PID控制器状态，避免积分项累积导致失控
    t12_pid.setpoint = temperature;
    t12_pid.integral = 0.0;               // 清空积分项
    t12_pid.prev_error = 0.0;             // 清空上次误差
    t12_pid.last_time = HAL_GetTick();    // 更新时间戳
    
    // 重置专注加热模式
    focused_heating_mode = 0;
    
    // 对于高温校准（>400°C），调整PID参数以获得更好的控制性能
    if (temperature > 400.0f) {
        // 高温时使用更保守的PID参数，避免振荡
        t12_pid.kp = 8.0f;   // 降低比例系数
        t12_pid.ki = 2.0f;   // 降低积分系数
        t12_pid.kd = 5.0f;   // 适当增加微分系数
        printf("[CalibrationTemp] High temperature PID parameters applied\n");
    } else {
        // 正常温度使用标准参数
        t12_pid.kp = 15.0f;
        t12_pid.ki = 5.0f;
        t12_pid.kd = 8.0f;
    }
    
    printf("[CalibrationTemp] PID reset for calibration: kp=%.1f, ki=%.1f, kd=%.1f\n", 
           t12_pid.kp, t12_pid.ki, t12_pid.kd);
}

/**
 * @brief 停止校准系统专用加热控制
 */
void StopCalibrationHeating(void) {
    if (heating_status) {
        heating_status = 0;
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);  // 停止PWM输出
        stopHeatingControlTimer();
    }
}

/**
 * @brief 初始化校准系统硬件抽象层
 * 
 * 这个函数应该在主系统初始化时调用，用于设置校准系统的硬件接口
 */
void CalibrationHardware_Init(void) {
    printf("[CalibrationHardware] Initializing calibration hardware abstraction layer...\n");
    
    // 这里可以添加任何硬件相关的初始化代码
    // 例如：初始化ADC、PWM、定时器等
    
    printf("[CalibrationHardware] Hardware abstraction layer initialized\n");
}