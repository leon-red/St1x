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

// 全局硬件接口指针（用于内部函数访问）
static const CalibrationHardwareInterface* g_hw_interface = NULL;

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
    
    // 通过硬件抽象层接口获取温度传感器参数，确保与主系统完全一致
    float mV_per_degree = 0.0033f;    // 默认值，如果接口可用则使用接口值
    float cold_junction_temp = 0;     // 默认值，如果接口可用则使用接口值
    
    if (g_hw_interface && g_hw_interface->getThermalVoltage && g_hw_interface->getColdJunctionTemp) {
        mV_per_degree = g_hw_interface->getThermalVoltage();
        cold_junction_temp = g_hw_interface->getColdJunctionTemp();
        printf("[CalibrationTemp] Using HAL parameters: mV/°C=%.4f, ColdJunction=%.1f°C\n", 
               mV_per_degree, cold_junction_temp);
    } else {
        printf("[CalibrationTemp] Using default parameters: mV/°C=%.4f, ColdJunction=%.1f°C\n", 
               mV_per_degree, cold_junction_temp);
    }
    
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

// PID参数获取接口函数实现
static float getPID_Kp(void) {
    extern PID_Controller t12_pid;
    return t12_pid.kp;
}

static float getPID_Ki(void) {
    extern PID_Controller t12_pid;
    return t12_pid.ki;
}

static float getPID_Kd(void) {
    extern PID_Controller t12_pid;
    return t12_pid.kd;
}

static uint8_t getHeatingStatus(void) {
    extern uint8_t heating_status;
    return heating_status;
}

static uint8_t getFocusedHeatingMode(void) {
    extern uint8_t focused_heating_mode;
    return focused_heating_mode;
}

static void setFocusedHeatingMode(uint8_t mode) {
    extern uint8_t focused_heating_mode;
    focused_heating_mode = mode;
}

// 温度限制管理接口函数实现
static void setTemperatureLimit(float limit) {
    extern float max_temperature_limit;
    max_temperature_limit = limit;
}

static float getTemperatureLimit(void) {
    extern float max_temperature_limit;
    return max_temperature_limit;
}

// 温度传感器参数获取接口函数实现
static float getThermalVoltage(void) {
    // 从主系统获取温度传感器参数，确保参数一致性
    extern float getThermalVoltageParameter(void);  // 从St1xADC.c中获取参数函数
    return getThermalVoltageParameter();
}

static float getColdJunctionTemp(void) {
    // 从主系统获取冷端补偿参数，确保参数一致性
    extern float getColdJunctionTempParameter(void);  // 从St1xADC.c中获取参数函数
    return getColdJunctionTempParameter();
}

// 系统配置参数获取接口函数实现（仅同步需要与主系统一致的参数）
static uint32_t getKeyDebounceTime(void) {
    // 从主系统获取按键去抖时间配置
    extern uint32_t getKeyDebounceTimeParameter(void);  // 从St1xKey.c中获取参数函数
    return getKeyDebounceTimeParameter();
}

static uint32_t getKeyLongPressTime(void) {
    // 从主系统获取按键长按时间配置
    extern uint32_t getKeyLongPressTimeParameter(void);  // 从St1xKey.c中获取参数函数
    return getKeyLongPressTimeParameter();
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
    .stopHeatingTimer = stopHeatingControlTimer,
    
    // PID参数获取接口
    .getPID_Kp = getPID_Kp,
    .getPID_Ki = getPID_Ki,
    .getPID_Kd = getPID_Kd,
    .getHeatingStatus = getHeatingStatus,
    .getFocusedHeatingMode = getFocusedHeatingMode,
    .setFocusedHeatingMode = setFocusedHeatingMode,
    
    // 温度限制管理接口
    .setTemperatureLimit = setTemperatureLimit,
    .getTemperatureLimit = getTemperatureLimit,
    
    // 温度传感器参数获取接口
    .getThermalVoltage = getThermalVoltage,
    .getColdJunctionTemp = getColdJunctionTemp,
    
    // 温度限制管理接口
    .setTemperatureLimit = setTemperatureLimit,
    .getTemperatureLimit = getTemperatureLimit,
    
    // 系统配置参数获取接口（仅同步需要与主系统一致的参数）
    .getKeyDebounceTime = getKeyDebounceTime,
    .getKeyLongPressTime = getKeyLongPressTime
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
    if (g_hw_interface && g_hw_interface->getHeatingStatus && 
        !g_hw_interface->getHeatingStatus()) {
        
        // 通过硬件抽象层接口设置加热状态
        if (g_hw_interface->setFocusedHeatingMode) {
            g_hw_interface->setFocusedHeatingMode(0);  // 重置专注加热模式
        }
        
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);  // 启动PWM输出
        startHeatingControlTimer();
    }
}

/**
 * @brief 校准系统专用温度设置函数
 * 
 * 这个函数用于校准系统，设置目标温度并重置PID状态
 * 特别注意：通过硬件抽象层接口获取主系统当前的PID参数
 * 
 * @param temperature 目标温度
 */
void setTemperatureForCalibration(float temperature) {
    printf("[CalibrationTemp] Setting calibration temperature to %.1f°C\n", temperature);
    
    // 重置PID控制器状态，避免积分项累积导致失控
    extern PID_Controller t12_pid;
    t12_pid.setpoint = temperature;
    t12_pid.integral = 0.0;               // 清空积分项
    t12_pid.prev_error = 0.0;             // 清空上次误差
    t12_pid.last_time = HAL_GetTick();    // 更新时间戳
    
    // 通过硬件抽象层接口重置专注加热模式
    if (g_hw_interface && g_hw_interface->setFocusedHeatingMode) {
        g_hw_interface->setFocusedHeatingMode(0);
    }
    
    // 获取当前温度（用于计算误差）
    float current_temp = getCalibrationTemperature();
    float error = temperature - current_temp;
    
    // 通过硬件抽象层接口获取PID参数
    if (g_hw_interface && g_hw_interface->getPID_Kp && 
        g_hw_interface->getPID_Ki && g_hw_interface->getPID_Kd) {
        
        float kp = g_hw_interface->getPID_Kp();
        float ki = g_hw_interface->getPID_Ki();
        float kd = g_hw_interface->getPID_Kd();
        
        printf("[CalibrationTemp] Using main system PID parameters via HAL: kp=%.3f, ki=%.3f, kd=%.3f\n", 
               kp, ki, kd);
    } else {
        printf("[CalibrationTemp] Using main system PID parameters directly: kp=%.3f, ki=%.3f, kd=%.3f\n", 
               t12_pid.kp, t12_pid.ki, t12_pid.kd);
    }
    
    printf("[CalibrationTemp] Temperature error: %.1f°C\n", error);
}

/**
 * @brief 停止校准系统专用加热控制
 */
void StopCalibrationHeating(void) {
    if (g_hw_interface && g_hw_interface->getHeatingStatus && 
        g_hw_interface->getHeatingStatus()) {
        
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);  // 停止PWM输出
        stopHeatingControlTimer();
    }
}

/**
 * @brief 同步主系统的PID参数到校准系统
 * 
 * 这个函数用于确保校准系统使用与主系统完全相同的PID参数
 * 当主系统PID参数调整时，调用此函数即可自动同步
 */
void SyncPIDParametersFromMainSystem(void) {
    printf("[CalibrationHardware] Synchronizing PID parameters from main system...\n");
    
    // 通过硬件抽象层接口获取主系统当前的PID参数
    if (g_hw_interface && g_hw_interface->getPID_Kp && 
        g_hw_interface->getPID_Ki && g_hw_interface->getPID_Kd) {
        
        float kp = g_hw_interface->getPID_Kp();
        float ki = g_hw_interface->getPID_Ki();
        float kd = g_hw_interface->getPID_Kd();
        
        printf("[CalibrationHardware] Main system PID parameters via HAL: kp=%.3f, ki=%.3f, kd=%.3f\n", 
               kp, ki, kd);
    } else {
        // 备用方案：直接获取PID参数
        extern PID_Controller t12_pid;
        printf("[CalibrationHardware] Main system PID parameters directly: kp=%.3f, ki=%.3f, kd=%.3f\n", 
               t12_pid.kp, t12_pid.ki, t12_pid.kd);
    }
    
    // 校准系统会自动使用这些参数，因为共享同一个PID控制器实例
    printf("[CalibrationHardware] PID parameters synchronized successfully\n");
}

/**
 * @brief 初始化校准系统硬件抽象层
 * 
 * 这个函数应该在主系统初始化时调用，用于设置校准系统的硬件接口
 */
void CalibrationHardware_Init(void) {
    printf("[CalibrationHardware] Initializing calibration hardware abstraction layer...\n");
    
    // 设置全局硬件接口指针
    g_hw_interface = GetCalibrationHardwareInterface();
    
    // 这里可以添加任何硬件相关的初始化代码
    // 例如：初始化ADC、PWM、定时器等
    
    printf("[CalibrationHardware] Hardware abstraction layer initialized\n");
}