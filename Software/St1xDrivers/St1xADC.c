// ====================================================
// 开源地址：https://github.com/leon-red/St1x
// 作者：Leon Red
// 项目启动时间：2023/2/13
// ====================================================

#include "St1xADC.h"
#include "St1xPID.h"
#include "adc.h"
#include "tim.h"
#include <math.h>

// 系统配置
#define TEMP_FILTER_SIZE 4           // PID控制用滤波器（快速响应）
#define DISPLAY_FILTER_SIZE 8        // 显示用滤波器（平滑显示）
#define USB_VOLTAGE_THRESHOLD 15.0f
#define CONTROL_INTERVAL 50
#define CHIP_TEMP_FILTER_SIZE 8

// 全局变量
static float ATemp = 0;
float max_temperature_limit = NORMAL_TEMPERATURE_LIMIT;
float target_temperature = 360.0f;
uint8_t heating_status = 0;
uint16_t DMA_ADC[3] = {0};

// 温度滤波器 - 用于PID控制（快速响应）
static float temperature_buffer[TEMP_FILTER_SIZE] = {0};
static uint8_t filter_index = 0;
static uint8_t filter_initialized = 0;
float filtered_temperature = 0;     // PID控制用温度

// 显示温度滤波器 - 用于界面显示（平滑显示）
static float display_temp_buffer[DISPLAY_FILTER_SIZE] = {0};
static uint8_t display_filter_index = 0;
static uint8_t display_filter_initialized = 0;
float display_temperature = 0;      // 显示用温度

// 芯片温度滤波器
static float chip_temp_buffer[CHIP_TEMP_FILTER_SIZE] = {0};
static uint8_t chip_temp_index = 0;
static uint8_t chip_temp_initialized = 0;
float ambient_temperature = 25.0f;

// 系统状态标志
uint8_t adc_sampling_flag = 0;      // ADC采样状态标志
uint8_t heating_control_enabled = 0; // PID控制使能标志

// 时间记录
uint32_t last_control_time = 0;
static uint32_t last_status_check = 0;
uint32_t heating_start_time = 0;

// 采样状态机变量（供外部模块访问）
uint8_t sampling_phase = 0;
uint32_t sample_start_time = 0;
uint16_t saved_pwm_value = 0;

/**
 * 初始化冷端补偿温度
 */
void initializeColdJunctionTemperature(void) {
    HAL_Delay(500);
    
    float sum_temp = 0;
    for (uint8_t i = 0; i < 10; i++) {
        sum_temp += getChipInternalTemperature();
        HAL_Delay(100);
    }
    
    ATemp = (sum_temp / 10.0f) - 6.0f;
    if (ATemp < -20.0f) ATemp = -20.0f;
    if (ATemp > 60.0f) ATemp = 60.0f;
}

/**
 * ADC值到温度转换
 */
float calculateT12Temperature(uint16_t adcValue) {
    if (adcValue > ADC_MAX_VALUE) adcValue = ADC_MAX_VALUE;
    
    float voltage = (adcValue * ADC_REFERENCE_VOLTAGE) / ADC_MAX_VALUE;
    float temperature = voltage / THERMAL_VOLTAGE_PARAMETER + ATemp;
    
    if (temperature < 0) temperature = 0;
    if (temperature > max_temperature_limit) temperature = max_temperature_limit;
    
    return temperature;
}

/**
 * 更新PID控制温度滤波器（快速响应）
 */
void updateTemperatureFilter(uint16_t adcValue) {
    float current_temp = calculateT12Temperature(adcValue);
    
    // PID控制用滤波器
    temperature_buffer[filter_index] = current_temp;
    filter_index = (filter_index + 1) % TEMP_FILTER_SIZE;
    
    if (!filter_initialized) {
        for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
            temperature_buffer[i] = current_temp;
        }
        filter_initialized = 1;
    }
    
    float sum = 0;
    for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
        sum += temperature_buffer[i];
    }
    filtered_temperature = sum / TEMP_FILTER_SIZE;
    
    // 显示用滤波器（更平滑）
    display_temp_buffer[display_filter_index] = current_temp;
    display_filter_index = (display_filter_index + 1) % DISPLAY_FILTER_SIZE;
    
    if (!display_filter_initialized) {
        for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
            display_temp_buffer[i] = current_temp;
        }
        display_filter_initialized = 1;
    }
    
    float display_sum = 0;
    for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
        display_sum += display_temp_buffer[i];
    }
    display_temperature = display_sum / DISPLAY_FILTER_SIZE;
}

/**
 * USB电压检测
 */
uint8_t isUSBVoltageSufficient(void) {
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.1515f;
    return (usb_voltage >= USB_VOLTAGE_THRESHOLD);
}

/**
 * 系统安全检查
 */
uint8_t checkSystemSafety(void) {
    // 电压检查
    if (!isUSBVoltageSufficient()) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        return 0;
    }
    
    // 温度检查
    if (filtered_temperature > max_temperature_limit || 
        filtered_temperature < 0.0f || 
        filtered_temperature > 500.0f) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        return 0;
    }
    
    return 1;
}

/**
 * 获取芯片内部温度
 */
float getChipInternalTemperature(void) {
    if (DMA_ADC[2] == 0) return 25.0f;
    
    uint16_t temp_adc = DMA_ADC[2];
    float vsense = (temp_adc * 3.3f) / 4095.0f;
    float temperature = ((CHIP_TEMP_V25 - vsense) * 1000.0f) / CHIP_TEMP_AVG_SLOPE + 25.0f;
    
    if (temperature < -40.0f) temperature = -40.0f;
    if (temperature > 125.0f) temperature = 125.0f;
    
    return temperature;
}

/**
 * 更新环境温度估计
 */
void updateAmbientTemperatureFilter(void) {
    float current_chip_temp = getChipInternalTemperature();
    
    chip_temp_buffer[chip_temp_index] = current_chip_temp;
    chip_temp_index = (chip_temp_index + 1) % CHIP_TEMP_FILTER_SIZE;
    
    if (!chip_temp_initialized) {
        for (uint8_t i = 0; i < CHIP_TEMP_FILTER_SIZE; i++) {
            chip_temp_buffer[i] = current_chip_temp;
        }
        chip_temp_initialized = 1;
    }
    
    float sum = 0;
    for (uint8_t i = 0; i < CHIP_TEMP_FILTER_SIZE; i++) {
        sum += chip_temp_buffer[i];
    }
    
    float chip_temp_filtered = sum / CHIP_TEMP_FILTER_SIZE;
    ambient_temperature = chip_temp_filtered - 4.0f;
    
    if (ambient_temperature < -20.0f) ambient_temperature = -20.0f;
    if (ambient_temperature > 60.0f) ambient_temperature = 60.0f;
}

/**
 * 系统状态监控
 */
void systemStatusMonitor(void) {
    uint32_t current_time = HAL_GetTick();
    
    if ((current_time - last_status_check) >= 100) {
        checkSystemSafety();
        updateAmbientTemperatureFilter();
        last_status_check = current_time;
    }
}

///**
// * 校准系统接口函数
// */
//void setCalibrationTemperature(float temperature) {
//    target_temperature = temperature;
//    if (temperature > max_temperature_limit) {
//        max_temperature_limit = CALIBRATION_TEMPERATURE_LIMIT;
//    }
//}
//
//void StopCalibrationHeating(void) {
//    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
//    stopHeatingControlTimer();
//    heating_status = 0;
//}
//
//void StartCalibrationHeating(void) {
//    startHeatingControlTimer();
//    heating_status = 1;
//    heating_start_time = HAL_GetTick();
//}
//
//// 简化后的空函数（保持接口兼容）
//uint8_t scanCalibrationKeys(void) { return 0; }
//void saveCalibrationData(float* offsets, uint8_t count) { (void)offsets; (void)count; }
//
//// 删除重复的变量声明