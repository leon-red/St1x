// ====================================================
// 开源地址：https://github.com/leon-red/St1x
// 作者：Leon Red
// 项目启动时间：2023/2/13
// ====================================================

#include "St1xADC.h"
#include "St1xPID.h"
#include "St1xFlash.h"
#include "tim.h"
#include <math.h>

// 系统配置
#define TEMP_FILTER_SIZE 4           // PID控制用滤波器（快速响应）
#define DISPLAY_FILTER_SIZE 8        // 显示用滤波器（平滑显示）
#define USB_VOLTAGE_THRESHOLD 15.0f
#define CONTROL_INTERVAL 50
#define CHIP_TEMP_FILTER_SIZE 8

// 全局变量
static float ATemp = -1.0f;
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
    HAL_Delay(200);
    
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
    // 检查ATemp是否已初始化
    if (ATemp < -0.5f) {
        // ATemp未初始化，调用初始化函数
        initializeColdJunctionTemperature();
    }
    
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
    
    // 温度检查 - 使用动态温度限制，而不是固定限制
    if (filtered_temperature > max_temperature_limit || 
        filtered_temperature < 0.0f) {
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

// 9点校准插值计算相关变量和函数
#define CALIBRATION_POINTS_COUNT 9

// 9点校准数据结构
typedef struct {
    uint16_t adc_value;    // ADC原始值
    float temperature;     // 对应的实际温度值
} CalibrationPoint;

// 9点校准数据数组（从FLASH读取）
static CalibrationPoint calibration_points[CALIBRATION_POINTS_COUNT];

// 校准偏移值数组（从FLASH读取）
static float calibration_offsets[CALIBRATION_POINTS_COUNT] = {0};

// 电压计算方式选择标志
// 根据宏定义自动初始化
#if USE_9POINT_CALIBRATION == 0
static uint8_t voltage_calculation_method = 1; // 9点插值
#else
static uint8_t voltage_calculation_method = 0; // 线性计算
#endif

// 初始化校准数据（从FLASH读取）
static void initializeCalibrationData(void) {
    // 定义9点校准温度点（与校准系统保持一致）
    static const float cal_temps[CALIBRATION_POINTS_COUNT] = {
        80.0f, 130.0f, 180.0f, 230.0f, 280.0f,
        330.0f, 380.0f, 430.0f, 480.0f
    };
    
    // 从FLASH加载校准偏移值
    if (St1xFlash_IsCalibrationDataValid()) {
        St1xFlash_LoadCalibrationData(calibration_offsets, CALIBRATION_POINTS_COUNT);
        
        // 应用偏移值到校准点温度
        // 校准点实际温度 = 校准系统目标温度 + 偏移值
        for (uint8_t i = 0; i < CALIBRATION_POINTS_COUNT; i++) {
            calibration_points[i].temperature = cal_temps[i] + calibration_offsets[i];
        }
    } else {
        // 如果没有校准数据，使用校准系统目标温度作为默认值
        for (uint8_t i = 0; i < CALIBRATION_POINTS_COUNT; i++) {
            calibration_offsets[i] = 0.0f;
            calibration_points[i].temperature = cal_temps[i];
        }
    }
    
    // 注意：ADC值需要在校准过程中实际测量得到
    // 这里暂时使用线性分布的ADC值作为占位符
    // 实际应用中，这些ADC值应该在校准过程中测量并保存
    for (uint8_t i = 0; i < CALIBRATION_POINTS_COUNT; i++) {
        calibration_points[i].adc_value = i * (4095 / (CALIBRATION_POINTS_COUNT - 1));
    }
}

// 9点插值计算函数
static float calculateTemperatureBy9PointInterpolation(uint16_t adcValue) {
    // 边界检查
    if (adcValue <= calibration_points[0].adc_value) {
        return calibration_points[0].temperature;
    }
    if (adcValue >= calibration_points[CALIBRATION_POINTS_COUNT-1].adc_value) {
        return calibration_points[CALIBRATION_POINTS_COUNT-1].temperature;
    }
    
    // 查找adcValue所在的区间
    uint8_t i;
    for (i = 0; i < CALIBRATION_POINTS_COUNT - 1; i++) {
        if (adcValue >= calibration_points[i].adc_value && 
            adcValue <= calibration_points[i+1].adc_value) {
            break;
        }
    }
    
    // 线性插值计算
    float ratio = (float)(adcValue - calibration_points[i].adc_value) / 
                  (float)(calibration_points[i+1].adc_value - calibration_points[i].adc_value);
    
    float temperature = calibration_points[i].temperature + 
                        ratio * (calibration_points[i+1].temperature - calibration_points[i].temperature);
    
    // 应用冷端补偿
    temperature += ATemp;
    
    // 温度范围限制
    if (temperature < 0) temperature = 0;
    if (temperature > max_temperature_limit) temperature = max_temperature_limit;
    
    return temperature;
}

// 选择电压计算方式
void selectVoltageCalculationMethod(uint8_t method) {
    if (method <= 1) {
        voltage_calculation_method = method;
        
        // 如果选择9点插值，初始化校准数据
        if (method == 1) {
            initializeCalibrationData();
        }
    }
}

// 获取当前使用的电压计算方式
uint8_t getCurrentVoltageCalculationMethod(void) {
    return voltage_calculation_method;
}

// 设置9点校准数据
void setCalibrationPoint(uint8_t index, uint16_t adc_value, float temperature) {
    if (index < CALIBRATION_POINTS_COUNT) {
        calibration_points[index].adc_value = adc_value;
        calibration_points[index].temperature = temperature;
    }
}

// 获取9点校准数据
void getCalibrationPoint(uint8_t index, uint16_t* adc_value, float* temperature) {
    if (index < CALIBRATION_POINTS_COUNT && adc_value != NULL && temperature != NULL) {
        *adc_value = calibration_points[index].adc_value;
        *temperature = calibration_points[index].temperature;
    }
}

// 获取校准偏移值
float getCalibrationOffset(uint8_t index) {
    if (index < CALIBRATION_POINTS_COUNT) {
        return calibration_offsets[index];
    }
    return 0.0f;
}

// 重新加载校准数据（当FLASH数据更新后调用）
void reloadCalibrationData(void) {
    initializeCalibrationData();
}

// 增强的温度计算函数（支持两种计算方式）
float calculateT12TemperatureEnhanced(uint16_t adcValue) {
    // 根据运行时选择的计算方式进行计算
    if (voltage_calculation_method == 1) {
        // 使用9点插值计算
        return calculateTemperatureBy9PointInterpolation(adcValue);
    } else {
        // 使用原有的线性计算
        return calculateT12Temperature(adcValue);
    }
}