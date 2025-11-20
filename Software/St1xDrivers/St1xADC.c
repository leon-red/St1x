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

// 冷端补偿优化配置
#define COLD_JUNCTION_CORRECTION_FACTOR 0.8f  // 冷端补偿修正系数
#define MIN_COLD_JUNCTION_TEMP -20.0f
#define MAX_COLD_JUNCTION_TEMP 60.0f

// 全局变量
static float ATemp = -1.0f;
float max_temperature_limit = NORMAL_TEMPERATURE_LIMIT;
float target_temperature = 360.0f;
uint8_t heating_status = 0;
uint16_t DMA_ADC[3] = {0};

// 新增系统状态变量
uint32_t system_start_time = 0;     // 系统启动时间
uint32_t last_heating_time = 0;     // 上次加热时间

// 增强的温度计算函数声明
float calculateT12TemperatureEnhancedWithEnvCompensation(uint16_t adcValue);

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

// 冷端补偿优化变量
static float cold_junction_offset = 0.0f;  // 冷端补偿偏移量
static uint8_t cold_junction_calibrated = 0; // 冷端补偿校准状态

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
 * 优化冷端补偿温度初始化
 * 针对外壳浮空连接的特殊补偿算法
 */
void initializeColdJunctionTemperature(void) {
    HAL_Delay(200);
    
    float sum_temp = 0;
    for (uint8_t i = 0; i < 10; i++) {
        sum_temp += getChipInternalTemperature();
        HAL_Delay(100);
    }
    
    float chip_temp_avg = sum_temp / 10.0f;
    
    // 针对外壳浮空连接的优化补偿算法
    // 外壳通过1M电阻+470pF连接到GND，冷端补偿需要特殊处理
    if (chip_temp_avg < 15.0f) {
        // 低温环境：外壳浮空影响较小，使用标准补偿
        ATemp = chip_temp_avg - 5.0f;
    } else if (chip_temp_avg > 35.0f) {
        // 高温环境：外壳浮空影响较大，增加补偿量
        ATemp = chip_temp_avg - 7.0f;
    } else {
        // 常温环境：中等补偿
        ATemp = chip_temp_avg - 6.0f;
    }
    
    // 应用冷端补偿修正系数
    ATemp *= COLD_JUNCTION_CORRECTION_FACTOR;
    
    // 温度范围限制
    if (ATemp < MIN_COLD_JUNCTION_TEMP) ATemp = MIN_COLD_JUNCTION_TEMP;
    if (ATemp > MAX_COLD_JUNCTION_TEMP) ATemp = MAX_COLD_JUNCTION_TEMP;
    
    cold_junction_calibrated = 1;
}

/**
 * 动态冷端补偿调整
 * 根据系统状态实时调整冷端补偿值
 */
void adjustColdJunctionCompensation(void) {
    static uint32_t last_adjust_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每30秒调整一次冷端补偿
    if ((current_time - last_adjust_time) < 30000) {
        return;
    }
    
    float chip_temp = getChipInternalTemperature();
    
    // 根据系统状态动态调整冷端补偿
    if (!heating_status) {
        // 未加热状态：芯片温度接近环境温度
        if ((current_time - last_heating_time) > 300000) { // 5分钟未加热
            // 长时间未加热，芯片温度更接近环境温度
            cold_junction_offset = -2.0f;
        } else {
            // 刚停止加热，芯片温度仍较高
            cold_junction_offset = -4.0f;
        }
    } else {
        // 加热状态：芯片温度显著高于环境温度
        cold_junction_offset = -6.0f;
    }
    
    // 根据芯片温度微调补偿
    if (chip_temp > 45.0f) {
        cold_junction_offset -= 1.0f; // 芯片温度高，增加补偿
    } else if (chip_temp < 20.0f) {
        cold_junction_offset += 1.0f; // 芯片温度低，减少补偿
    }
    
    last_adjust_time = current_time;
}

/**
 * 获取优化后的冷端补偿温度
 */
float getOptimizedColdJunctionTemperature(void) {
    if (!cold_junction_calibrated) {
        initializeColdJunctionTemperature();
    }
    
    // 动态调整冷端补偿
    adjustColdJunctionCompensation();
    
    float optimized_atemp = ATemp + cold_junction_offset;
    
    // 范围限制
    if (optimized_atemp < MIN_COLD_JUNCTION_TEMP) optimized_atemp = MIN_COLD_JUNCTION_TEMP;
    if (optimized_atemp > MAX_COLD_JUNCTION_TEMP) optimized_atemp = MAX_COLD_JUNCTION_TEMP;
    
    return optimized_atemp;
}

/**
 * ADC值到温度转换（基础版本）
 */
float calculateT12Temperature(uint16_t adcValue) {
    if (adcValue > ADC_MAX_VALUE) adcValue = ADC_MAX_VALUE;
    
    float voltage = (adcValue * ADC_REFERENCE_VOLTAGE) / ADC_MAX_VALUE;
    
    // 使用优化后的冷端补偿温度
    float optimized_atemp = getOptimizedColdJunctionTemperature();
    
    float temperature = voltage / THERMAL_VOLTAGE_PARAMETER + optimized_atemp;
    
    if (temperature < 0) temperature = 0;
    if (temperature > max_temperature_limit) temperature = max_temperature_limit;
    
    return temperature;
}

/**
 * 更新PID控制温度滤波器（快速响应）
 */
void updateTemperatureFilter(uint16_t adcValue) {
    // 使用增强的温度计算函数，考虑环境温度补偿
    float current_temp = calculateT12TemperatureEnhancedWithEnvCompensation(adcValue);
    
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
// 智能环境温度感知和热电偶校准系统
#define ENVIRONMENT_TEMP_SENSOR_ENABLED 1
// 删除重复的宏定义
// #define ENVIRONMENT_TEMP_SAMPLING_INTERVAL 300000  // 5分钟采样间隔
#define ENVIRONMENT_TEMP_CALIBRATION_POINTS 3      // 环境温度校准点数量

// 环境温度传感器数据结构
typedef struct {
    float chip_temp;           // 芯片内部温度
    float estimated_env_temp;  // 估算的环境温度
    uint32_t last_sample_time; // 上次采样时间
    uint8_t calibration_count; // 校准点计数
    float calibration_temps[ENVIRONMENT_TEMP_CALIBRATION_POINTS]; // 环境温度校准点
} EnvironmentTempSensor;

static EnvironmentTempSensor env_temp_sensor = {0};

// 热电偶非线性校准参数
typedef struct {
    float base_coeff;          // 基础系数
    float temp_coeff;          // 温度相关系数
    float env_temp_coeff;      // 环境温度相关系数
} ThermocoupleCalibration;

static ThermocoupleCalibration thermocouple_cal = {0.0044f, 0.00001f, 0.00005f};

/**
 * 智能估算环境温度
 * 基于芯片温度、系统状态和加热历史估算真实环境温度
 */
float estimateEnvironmentTemperature(void) {
    float chip_temp = getChipInternalTemperature();
    
    // 如果系统刚启动且未加热，芯片温度接近环境温度
    if (!heating_status && (HAL_GetTick() - system_start_time) < 60000) {
        return chip_temp - 2.0f; // 芯片温度略高于环境温度
    }
    
    // 如果系统长时间未加热，芯片温度会逐渐接近环境温度
    if (!heating_status && (HAL_GetTick() - last_heating_time) > 300000) {
        return chip_temp - 1.5f;
    }
    
    // 默认情况下，基于芯片温度和系统状态估算
    float base_env_temp = chip_temp - 4.0f; // 芯片通常比环境高4°C
    
    // 根据加热状态调整估算
    if (heating_status) {
        base_env_temp -= 2.0f; // 加热时芯片温度更高
    }
    
    // 限制环境温度范围
    if (base_env_temp < -20.0f) base_env_temp = -20.0f;
    if (base_env_temp > 60.0f) base_env_temp = 60.0f;
    
    return base_env_temp;
}

/**
 * 动态更新热电偶校准参数
 * 根据环境温度和历史数据自动调整热电偶校准参数
 */
void updateThermocoupleCalibration(float env_temp) {
    // 环境温度对热电偶特性的影响
    // 温度越低，热电偶灵敏度略有下降
    if (env_temp < 10.0f) {
        thermocouple_cal.env_temp_coeff = 0.00008f; // 低温环境补偿
    } else if (env_temp > 30.0f) {
        thermocouple_cal.env_temp_coeff = 0.00002f; // 高温环境补偿
    } else {
        thermocouple_cal.env_temp_coeff = 0.00005f; // 常温环境
    }
    
    // 根据环境温度调整基础系数
    thermocouple_cal.base_coeff = 0.0044f + (env_temp - 25.0f) * 0.000002f;
}

/**
 * 增强的热电偶温度计算函数
 * 考虑环境温度和非线性特性的智能温度计算
 */
float calculateT12TemperatureEnhancedWithEnvCompensation(uint16_t adcValue) {
    // 估算环境温度
    float env_temp = estimateEnvironmentTemperature();
    
    // 更新热电偶校准参数
    updateThermocoupleCalibration(env_temp);
    
    if (adcValue > ADC_MAX_VALUE) adcValue = ADC_MAX_VALUE;
    
    float voltage = (adcValue * ADC_REFERENCE_VOLTAGE) / ADC_MAX_VALUE;
    
    // 智能温度计算，考虑非线性特性和环境温度影响
    float base_temperature = voltage / thermocouple_cal.base_coeff;
    
    // 非线性补偿
    float nonlinear_compensation = base_temperature * base_temperature * thermocouple_cal.temp_coeff;
    
    // 环境温度补偿
    float env_compensation = (env_temp - 25.0f) * thermocouple_cal.env_temp_coeff * base_temperature;
    
    // 使用优化后的冷端补偿温度
    float optimized_atemp = getOptimizedColdJunctionTemperature();
    
    // 综合温度计算
    float temperature = base_temperature + nonlinear_compensation + env_compensation + optimized_atemp;
    
    // 温度范围限制
    if (temperature < 0) temperature = 0;
    if (temperature > max_temperature_limit) temperature = max_temperature_limit;
    
    return temperature;
}

/**
 * 环境温度感知系统状态监控
 */
void environmentTemperatureMonitor(void) {
    static uint32_t last_env_check = 0;
    uint32_t current_time = HAL_GetTick();
    
    if ((current_time - last_env_check) >= ENVIRONMENT_TEMP_SAMPLING_INTERVAL) {
        // 更新环境温度估算
        env_temp_sensor.chip_temp = getChipInternalTemperature();
        env_temp_sensor.estimated_env_temp = estimateEnvironmentTemperature();
        env_temp_sensor.last_sample_time = current_time;
        
        last_env_check = current_time;
    }
}

/**
 * 获取当前估算的环境温度
 */
// 删除重复的函数定义
// float getEstimatedEnvironmentTemperature(void) {
//     return env_temp_sensor.estimated_env_temp;
// }

/**
 * 获取热电偶校准状态信息
 */
void getThermocoupleCalibrationInfo(float* base_coeff, float* temp_coeff, float* env_coeff) {
    if (base_coeff) *base_coeff = thermocouple_cal.base_coeff;
    if (temp_coeff) *temp_coeff = thermocouple_cal.temp_coeff;
    if (env_coeff) *env_coeff = thermocouple_cal.env_temp_coeff;
}

// 更新系统状态监控函数，加入环境温度监控
void systemStatusMonitor(void) {
    uint32_t current_time = HAL_GetTick();
    
    if ((current_time - last_status_check) >= 100) {
        checkSystemSafety();
        updateAmbientTemperatureFilter();
        updateEnvironmentTemperatureFilter(); // 新增环境温度监控
        environmentTemperatureMonitor(); // 智能环境温度监控
        last_status_check = current_time;
    }
}

/**
 * 系统初始化时调用环境温度传感器初始化
 */
void St1xADC_Init(void) {
    // 初始化冷端补偿温度
    initializeColdJunctionTemperature();
    
    // 初始化环境温度传感器
    initEnvironmentTemperatureSensor();
    
    // 其他初始化代码...
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

// 冷启动相关配置
#define COLD_START_TEMP_THRESHOLD 50.0f    // 冷启动温度阈值
#define COLD_START_DURATION 30000         // 冷启动持续时间（30秒）
#define COLD_START_AGGRESSIVE_KP 15.0f    // 冷启动激进比例系数

// 冷启动状态变量
static uint8_t cold_start_mode = 0;
static uint32_t cold_start_start_time = 0;

/**
 * 系统启动时初始化冷启动状态
 */
void initializeColdStartState(void) {
    cold_start_start_time = HAL_GetTick();
    cold_start_mode = 1; // 默认进入冷启动模式
}

/**
 * 检查并进入冷启动模式
 */
void checkAndEnterColdStartMode(float current_temp) {
    uint32_t current_time = HAL_GetTick();
    
    // 如果当前温度低于阈值且系统刚启动不久，进入冷启动模式
    if (current_temp < COLD_START_TEMP_THRESHOLD && 
        (current_time - cold_start_start_time) < COLD_START_DURATION) {
        cold_start_mode = 1;
    } else {
        cold_start_mode = 0;
    }
}

/**
 * 动态更新冷端补偿温度（考虑环境温度变化）
 */
void updateColdJunctionTemperature(void) {
    static uint32_t last_update_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每5分钟更新一次冷端补偿温度
    if ((current_time - last_update_time) >= 300000) { // 5分钟
        float sum_temp = 0;
        for (uint8_t i = 0; i < 5; i++) {
            sum_temp += getChipInternalTemperature();
            HAL_Delay(200);
        }
        
        ATemp = (sum_temp / 5.0f) - 6.0f;
        if (ATemp < -20.0f) ATemp = -20.0f;
        if (ATemp > 60.0f) ATemp = 60.0f;
        
        last_update_time = current_time;
    }
}

/**
 * 获取冷启动模式状态
 */
uint8_t isColdStartMode(void) {
    return cold_start_mode;
}

// 环境温度传感器配置
#define EXTERNAL_TEMP_SENSOR_ENABLED 0  // 设置为1启用外部温度传感器
#define ENVIRONMENT_TEMP_SAMPLING_INTERVAL 30000  // 环境温度采样间隔(ms)
#define ENVIRONMENT_TEMP_FILTER_SIZE 8           // 环境温度滤波器大小

// 环境温度传感器实例
static EnvironmentSensor env_sensor = {
    .sensor_type = TEMP_SENSOR_CHIP_INTERNAL,
    .current_temperature = 25.0f,
    .filtered_temperature = 25.0f,
    .last_sample_time = 0,
    .sensor_available = 1
};

// 环境温度滤波器
static float env_temp_buffer[ENVIRONMENT_TEMP_FILTER_SIZE] = {0};
static uint8_t env_temp_index = 0;
static uint8_t env_temp_initialized = 0;

/**
 * 初始化环境温度传感器
 */
void initEnvironmentTemperatureSensor(void) {
    // 根据配置选择传感器类型
    if (EXTERNAL_TEMP_SENSOR_ENABLED) {
        // 检查外部传感器是否可用
        // 这里可以添加DS18B20或NTC传感器的初始化代码
        env_sensor.sensor_available = 0; // 暂时设置为不可用
    } else {
        // 使用芯片内部温度传感器
        env_sensor.sensor_type = TEMP_SENSOR_CHIP_INTERNAL;
        env_sensor.sensor_available = 1;
    }
    
    // 初始化环境温度滤波器
    float initial_temp = getChipInternalTemperature() - 4.0f;
    for (uint8_t i = 0; i < ENVIRONMENT_TEMP_FILTER_SIZE; i++) {
        env_temp_buffer[i] = initial_temp;
    }
    env_temp_initialized = 1;
    env_sensor.filtered_temperature = initial_temp;
}

/**
 * 获取真实环境温度（如果外部传感器可用）
 */
float getRealEnvironmentTemperature(void) {
    if (!env_sensor.sensor_available) {
        // 传感器不可用，返回估算值
        return getEstimatedEnvironmentTemperature();
    }
    
    // 根据传感器类型获取温度
    switch (env_sensor.sensor_type) {
        case TEMP_SENSOR_EXTERNAL_DS18B20:
            // DS18B20温度读取实现
            // return ds18b20_read_temperature();
            break;
            
        case TEMP_SENSOR_EXTERNAL_NTC:
            // NTC热敏电阻温度读取实现
            // return ntc_read_temperature();
            break;
            
        case TEMP_SENSOR_CHIP_INTERNAL:
        default:
            // 使用芯片内部温度传感器估算
            return getChipInternalTemperature() - 4.0f;
    }
    
    return getEstimatedEnvironmentTemperature();
}

/**
 * 更新环境温度滤波器
 */
void updateEnvironmentTemperatureFilter(void) {
    static uint32_t last_update_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 检查采样间隔
    if ((current_time - last_update_time) < ENVIRONMENT_TEMP_SAMPLING_INTERVAL) {
        return;
    }
    
    // 获取当前环境温度
    float current_temp = getRealEnvironmentTemperature();
    env_sensor.current_temperature = current_temp;
    
    // 更新滤波器
    env_temp_buffer[env_temp_index] = current_temp;
    env_temp_index = (env_temp_index + 1) % ENVIRONMENT_TEMP_FILTER_SIZE;
    
    if (!env_temp_initialized) {
        for (uint8_t i = 0; i < ENVIRONMENT_TEMP_FILTER_SIZE; i++) {
            env_temp_buffer[i] = current_temp;
        }
        env_temp_initialized = 1;
    }
    
    // 计算滤波后温度
    float sum = 0;
    for (uint8_t i = 0; i < ENVIRONMENT_TEMP_FILTER_SIZE; i++) {
        sum += env_temp_buffer[i];
    }
    env_sensor.filtered_temperature = sum / ENVIRONMENT_TEMP_FILTER_SIZE;
    
    // 更新环境温度全局变量
    ambient_temperature = env_sensor.filtered_temperature;
    
    last_update_time = current_time;
    env_sensor.last_sample_time = current_time;
}

/**
 * 获取当前估算的环境温度
 */
// 保留这个函数定义，删除上面的重复定义
float getEstimatedEnvironmentTemperature(void) {
    return env_sensor.filtered_temperature;
}

/**
 * 设置环境传感器类型
 */
void setEnvironmentSensorType(TempSensorType sensor_type) {
    env_sensor.sensor_type = sensor_type;
    
    // 重新初始化传感器
    if (sensor_type == TEMP_SENSOR_CHIP_INTERNAL) {
        env_sensor.sensor_available = 1;
    } else {
        // 外部传感器需要检测可用性
        env_sensor.sensor_available = 0; // 暂时设置为不可用
    }
}

/**
 * 获取当前环境传感器类型
 */
TempSensorType getCurrentEnvironmentSensorType(void) {
    return env_sensor.sensor_type;
}

/**
 * 检查外部温度传感器是否可用
 */
uint8_t isExternalTempSensorAvailable(void) {
    return (env_sensor.sensor_type != TEMP_SENSOR_CHIP_INTERNAL) && 
           env_sensor.sensor_available;
}