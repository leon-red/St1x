// ====================================================
// 开源地址：https://github.com/leon-red/St1x
// 作者：Leon Red
// 项目启动时间：2023/2/13
// ====================================================

// ==================== 模块一：头文件包含 ====================
// 系统功能模块依赖声明
#include "St1xADC.h"    // ADC模块主头文件
#include "St1xPID.h"    // PID温度控制模块
#include "adc.h"        // STM32 HAL ADC驱动
#include "tim.h"        // 定时器驱动
#include <math.h>       // 数学函数库

// ==================== 模块二：系统常量配置 ====================

// 温度传感器参数配置
static float ATemp = 0;              // 环境温度补偿值（冷端补偿），初始为0，上电后更新为实际环境温度
#define Thermal_Voltage 0.0033f // 热电偶电压-温度转换系数（mV/°C）

// 温度传感器参数导出函数（供校准系统使用）
float getThermalVoltageParameter(void) {
    return Thermal_Voltage;
}

float getColdJunctionTempParameter(void) {
    return ATemp;
}

// ADC参数导出函数（供校准系统使用）
float getADCReferenceVoltageParameter(void) {
    return 3.3f;  // ADC参考电压
}

uint16_t getADCMaxValueParameter(void) {
    return 4095;  // ADC最大值
}

uint16_t getTemperatureSensorADCValueParameter(void) {
    return DMA_ADC[0];  // 温度传感器ADC原始值
}

/**
 * initializeColdJunctionTemperature - 初始化冷端补偿温度
 * 
 * 功能：在系统启动时获取环境温度作为冷端补偿值
 * 特点：只在上电时执行一次，使用芯片内部温度传感器
 * 应用：为热电偶温度测量提供准确的环境温度补偿
 */
void initializeColdJunctionTemperature(void) {
    // 等待系统稳定（约1秒）
    HAL_Delay(1000);
    
    // 获取芯片内部温度（需要多次采样取平均）
    float sum_temp = 0;
    uint8_t sample_count = 10;
    
    for (uint8_t i = 0; i < sample_count; i++) {
        float chip_temp = getChipInternalTemperature();
        sum_temp += chip_temp;
        HAL_Delay(100); // 每次采样间隔100ms
    }
    
    // 计算平均芯片温度
    float avg_chip_temp = sum_temp / sample_count;
    
    // 根据实测结果，芯片温度比环境温度高3~5度
    // 使用4°C偏移量来估计实际环境温度
    ATemp = avg_chip_temp - 4.0f;
    
    // 环境温度范围限制
    if (ATemp < -20.0f) ATemp = -20.0f;
    if (ATemp > 60.0f) ATemp = 60.0f;
}

// 滤波器配置
#define TEMP_FILTER_SIZE 4      // 控制用温度滤波器窗口大小（快速响应）

// 系统安全参数
#define USB_VOLTAGE_THRESHOLD 15.0f  // USB电压最低工作阈值（低于此值停止加热）
#define CONTROL_INTERVAL 50          // PID控制周期（50ms）

// 调试系统配置
#define ADC_DEBUG_ENABLED 0          // ADC调试信息输出开关

// 条件编译调试宏
#if ADC_DEBUG_ENABLED
    #define ADC_DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define ADC_DEBUG_PRINTF(...) do {} while(0)
#endif

// 全局温度限制（可在校准模式修改）
float max_temperature_limit = NORMAL_TEMPERATURE_LIMIT;  // 最大允许温度限制

// ==================== 模块三：全局状态变量 ====================

// 用户设置状态
float target_temperature = 360.0f;  // 用户设定的目标温度值
uint8_t heating_status = 0;          // 当前加热状态（0=停止，1=加热）

// 传感器数据缓存
uint16_t DMA_ADC[3] = {0};           // DMA传输的ADC原始数据（通道0:温度，通道1:电压，通道2:保留）

// 时间记录变量
uint32_t last_control_time = 0;      // 上次PID控制执行时间
uint32_t last_key_press_time = 0;    // 上次按键按下时间
uint16_t last_key_pin = 0;           // 上次按下的按键引脚

// 系统状态标志
uint8_t adc_sampling_flag = 0;       // ADC采样状态标志
uint8_t heating_control_enabled = 0; // PID控制使能标志

// 加热过程记录
uint32_t heating_start_time = 0;     // 加热开始时间戳

// 温度校准参数结构
TemperatureCalibration t12_cal = {0.0f, 1.0f, 0, 4095}; // T12烙铁头校准参数

// 环境温度相关变量
float ambient_temperature = 25.0f;           // 环境温度估计值
float chip_temperature_filtered = 25.0f;     // 滤波后的芯片内部温度
static float chip_temp_buffer[8] = {0};      // 芯片温度滤波器缓冲区
static uint8_t chip_temp_index = 0;           // 芯片温度滤波器索引
static uint8_t chip_temp_initialized = 0;     // 芯片温度滤波器初始化标志

// ==================== 模块四：内部私有变量 ====================

// 控制用温度滤波器（快速响应，用于PID控制）
static float temperature_buffer[TEMP_FILTER_SIZE] = {0};  // 温度数据环形缓冲区
static uint8_t filter_index = 0;                          // 缓冲区写入索引
static uint8_t filter_initialized = 0;                    // 滤波器初始化标志
static float filtered_temperature = 0;                   // 滤波后的控制温度

// PWM加热控制相关
static uint32_t heating_control_interval = 50;  // 加热控制周期
static uint32_t pwm_falling_edge_time = 0;      // PWM下降沿时间记录
static uint8_t waiting_for_delay = 0;           // 延时等待标志

// ADC采样状态机
uint8_t sampling_phase = 0;              // 当前采样阶段
uint32_t sample_start_time = 0;          // 采样阶段开始时间
uint16_t saved_pwm_value = 0;            // 采样时保存的PWM值

// ==================== 第六步：实现各种功能 ====================

/**
 * calculateADCForTemperature - 温度到ADC值的反向计算（调试用）
 * 
 * 功能：根据目标温度计算预期的ADC原始值
 * 用途：调试和验证温度传感器校准
 * 原理：温度 → 电压 → ADC值
 * 
 * @param target_temp 目标温度值（°C）
 * @return 预期的ADC原始值（0-4095）
 */
float calculateADCForTemperature(float target_temp) {
    const float mV_per_degree = Thermal_Voltage;  // 热电偶灵敏度系数
    const float cold_junction_temp = ATemp;       // 冷端补偿温度
    const float adc_ref_voltage = 3.3f;            // ADC参考电压
    const uint16_t adc_max = 4095;                // ADC最大分辨率
    
    // 反向计算流程：温度 → 电压 → ADC值
    float voltage = (target_temp - cold_junction_temp) * mV_per_degree;
    float adc_value = (voltage * adc_max) / adc_ref_voltage;
    
    return adc_value;
}

/**
 * calculateT12Temperature - ADC值到温度的正向转换
 *
 * 功能：将ADC原始读数转换为实际温度值
 * 原理：ADC值 → 电压 → 温度 + 冷端补偿
 * 安全：自动限制温度在0°C到最大温度限制之间
 * 调试：包含150°C和250°C的调试输出
 *
 * @param adcValue ADC原始读数（0-4095）
 * @return 实际温度值（°C）
 */

float calculateT12Temperature(uint16_t adcValue) {
    // 传感器参数
    const float mV_per_degree = Thermal_Voltage;  // 每摄氏度对应的电压
    const float cold_junction_temp = ATemp;       // 环境温度补偿
    const float adc_ref_voltage = 3.3f;            // ADC参考电压
    const uint16_t adc_max = 4095;                // ADC最大值

    // 防止读数异常
    if (adcValue > adc_max) adcValue = adc_max;

    // 数字信号转电压值
    float voltage = (adcValue * adc_ref_voltage) / adc_max;

    // 电压值转温度值
    float temperature = voltage / mV_per_degree + cold_junction_temp;

    // 添加调试：检查150°C和250°C对应的ADC值
    if (fabs(temperature - 150.0) < 1.0) {
        float expected_adc_150 = calculateADCForTemperature(150.0f);
        ADC_DEBUG_PRINTF("DEBUG 150°C: Temp=%.2f, ADC=%d, Voltage=%.4f, Expected ADC=%.1f\n",
               temperature, adcValue, voltage, expected_adc_150);
    }
    if (fabs(temperature - 250.0) < 1.0) {
        float expected_adc_250 = calculateADCForTemperature(250.0f);
        ADC_DEBUG_PRINTF("DEBUG 250°C: Temp=%.2f, ADC=%d, Voltage=%.4f, Expected ADC=%.1f\n",
               temperature, adcValue, voltage, expected_adc_250);
    }

    // 温度范围限制（使用全局温度限制）
    if (temperature < 0) temperature = 0;
    if (temperature > max_temperature_limit) temperature = max_temperature_limit;

    return temperature;
}

// ==================== 模块七：温度滤波系统 ====================

/**
 * getFilteredTemperature - 获取控制用滤波温度
 * 
 * 功能：返回经过快速滤波处理的温度值，用于PID控制
 * 特点：响应速度快，稳定性好，适合实时控制
 * 
 * @return 滤波后的控制温度值（°C）
 */
float getFilteredTemperature(void) {
    return filtered_temperature;
}

/**
 * updateTemperatureFilter - 更新控制温度滤波器
 * 
 * 功能：将新ADC读数加入滤波器并更新控制温度
 * 原理：使用环形缓冲区实现移动平均滤波
 * 特点：4点滤波，快速响应，适合PID控制需求
 * 
 * @param adcValue 新的ADC原始读数
 */
void updateTemperatureFilter(uint16_t adcValue) {
    // 把传感器读数转换成温度值
    float current_temp = calculateT12Temperature(adcValue);
    
    // 添加调试：检查滤波器输入
    if (fabs(current_temp - 150.0) < 5.0f || fabs(current_temp - 250.0) < 5.0f) {
        ADC_DEBUG_PRINTF("FILTER IN: temp=%.2f, adc=%d\n", current_temp, adcValue);
    }
    
    // 把新温度值存入缓冲区
    temperature_buffer[filter_index] = current_temp;
    filter_index = (filter_index + 1) % TEMP_FILTER_SIZE;
    
    // 如果是第一次使用，用当前值填满整个缓冲区
    if (!filter_initialized) {
        for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
            temperature_buffer[i] = current_temp;
        }
        filter_initialized = 1;
        ADC_DEBUG_PRINTF("FILTER INIT: temp=%.2f\n", current_temp);
    }
    
    // 计算平均温度值
    float sum = 0;
    for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
        sum += temperature_buffer[i];
    }
    float new_filtered_temp = sum / TEMP_FILTER_SIZE;
    
    // 添加调试：检查滤波器输出
    if (fabs(new_filtered_temp - 150.0) < 5.0f || fabs(new_filtered_temp - 250.0) < 5.0f) {
        ADC_DEBUG_PRINTF("FILTER OUT: temp=%.2f, old=%.2f\n", new_filtered_temp, filtered_temperature);
    }
    
    filtered_temperature = new_filtered_temp;
}

// ==================== 模块八：系统安全检测 ====================

/**
 * isUSBVoltageSufficient - USB供电电压检测
 * 
 * 功能：检测USB输入电压是否满足加热要求
 * 原理：通过ADC通道1读取电压传感器数据，转换为实际电压值
 * 电压转换公式：ADC值 → 电压分压 → 实际USB电压
 * 
 * @return 1=电压足够（≥15V），0=电压不足
 */
uint8_t isUSBVoltageSufficient(void) {
    // ADC值转换为实际电压：3.3V参考电压，0.1515分压比
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.1515f;
    return (usb_voltage >= USB_VOLTAGE_THRESHOLD);
}

/**
 * checkUSBVoltage - USB电压安全检查与保护
 * 
 * 功能：检测电压异常并执行安全保护措施
 * 保护措施：停止PWM输出、关闭定时器、重置加热状态
 * 应用场景：USB供电不足或断开时自动保护
 * 
 * @return 1=电压正常，0=电压异常（已执行保护）
 */
uint8_t checkUSBVoltage(void) {
    // 电压不足时执行保护
    if (!isUSBVoltageSufficient()) {
        // 安全保护三步曲：停止加热 → 关闭控制 → 重置状态
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        return 0;
    }
    return 1;
}

/**
 * checkTemperatureSafety - 温度安全监控与保护
 * 
 * 功能：实时监控温度是否在安全范围内
 * 安全检测：温度超限保护和传感器异常检测
 * 保护机制：温度超过限制或传感器异常时自动停止加热
 * 
 * @return 1=温度安全，0=温度异常（已执行保护）
 */
uint8_t checkTemperatureSafety(void) {
    float current_temp = getFilteredTemperature();
    
    // 温度超限保护（使用全局可配置的温度限制）
    if (current_temp > max_temperature_limit) {
        // 执行温度保护：停止加热并记录日志
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        ADC_DEBUG_PRINTF("[TemperatureSafety] Temperature exceeded limit: %.1f°C > %.1f°C\n", 
               current_temp, max_temperature_limit);
        return 0;
    }
    
    // 传感器异常检测（温度值不合理）
    if (current_temp < 0.0f || current_temp > 500.0f) {
        // 传感器异常保护
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        ADC_DEBUG_PRINTF("[TemperatureSafety] Sensor abnormal: %.1f°C\n", current_temp);
        return 0;
    }
    
    return 1;
}

// ==================== 模块十：系统状态监控 ====================

/**
 * systemStatusMonitor - 系统状态实时监控
 * 
 * 功能：周期性检查系统安全状态
 * 监控内容：USB电压检测、温度安全保护
 * 执行频率：每100ms检查一次，确保系统安全
 * 安全机制：电压不足或温度异常时自动停止加热
 */
void systemStatusMonitor(void) {
    static uint32_t last_status_check = 0;
    uint32_t current_time = HAL_GetTick();

    // 提高系统状态检查频率到每100ms一次，以确保安全
    if ((current_time - last_status_check) >= 100) {
        // 检查系统状态
        checkUSBVoltage();
        checkTemperatureSafety();
        
        // 更新环境温度估计（每100ms更新一次）
        updateAmbientTemperatureFilter();
        
        last_status_check = current_time;
    }
}

// ==================== 模块十一：环境温度获取系统 ====================

/**
 * getChipInternalTemperature - 获取STM32芯片内部温度
 * 
 * 功能：读取STM32F1内部温度传感器，转换为实际温度值
 * 原理：使用内部温度传感器通道（ADC_CHANNEL_TEMPSENSOR）
 * 温度计算：基于STM32F1数据手册的温度传感器特性
 * 参考：STM32F103xx数据手册 - 内部温度传感器特性
 * 
 * @return 芯片内部温度值（°C）
 */
float getChipInternalTemperature(void) {
    // 检查DMA_ADC数组是否有效
    if (DMA_ADC[2] == 0) {
        return 25.0f; // 默认返回室温
    }
    
    // 获取内部温度传感器的ADC值（通道2）
    uint16_t temp_adc = DMA_ADC[2];
    
    // 计算温度（基于STM32F1数据手册）
    // 公式：温度 = (V25 - Vsense) / Avg_Slope + 25
    // 其中：V25 = 1.43V（25°C时的电压）
    //       Avg_Slope = 4.3mV/°C
    //       Vsense = (ADC值 * Vdda) / 4095
    
    // 假设Vdda = 3.3V（实际应用中可能需要校准）
    float vdda = 3.3f;
    float vsense = (temp_adc * vdda) / 4095.0f;
    float temperature = ((CHIP_TEMP_V25 - vsense) * 1000.0f) / CHIP_TEMP_AVG_SLOPE + 25.0f;
    
    // 温度范围限制
    if (temperature < -40.0f) temperature = -40.0f;
    if (temperature > 125.0f) temperature = 125.0f;
    
    return temperature;
}

/**
 * getFilteredChipTemperature - 获取滤波后的芯片温度
 * 
 * 功能：返回经过滤波处理的芯片内部温度值
 * 特点：使用8点移动平均滤波，响应较慢但稳定性好
 * 用途：适合用于环境温度估计和显示
 * 
 * @return 滤波后的芯片内部温度（°C）
 */
float getFilteredChipTemperature(void) {
    return chip_temperature_filtered;
}

/**
 * updateAmbientTemperatureFilter - 更新环境温度滤波器
 * 
 * 功能：更新芯片温度滤波器并估计环境温度
 * 原理：芯片温度与环境温度存在热耦合关系
 * 特点：使用8点移动平均滤波，缓慢响应环境温度变化
 * 应用：为烙铁头提供环境温度补偿参考
 */
void updateAmbientTemperatureFilter(void) {
    // 获取当前芯片内部温度
    float current_chip_temp = getChipInternalTemperature();
    
    // 更新芯片温度滤波器
    chip_temp_buffer[chip_temp_index] = current_chip_temp;
    chip_temp_index = (chip_temp_index + 1) % 8;
    
    // 如果是第一次使用，用当前值填满整个缓冲区
    if (!chip_temp_initialized) {
        for (uint8_t i = 0; i < 8; i++) {
            chip_temp_buffer[i] = current_chip_temp;
        }
        chip_temp_initialized = 1;
        ADC_DEBUG_PRINTF("CHIP TEMP FILTER INIT: temp=%.2f\n", current_chip_temp);
    }
    
    // 计算平均芯片温度
    float sum = 0;
    for (uint8_t i = 0; i < 8; i++) {
        sum += chip_temp_buffer[i];
    }
    chip_temperature_filtered = sum / 8.0f;
    
    // 估计环境温度：芯片温度减去4°C偏移（根据实际测试调整为3~5度平均值）
    ambient_temperature = chip_temperature_filtered - 4.0f;
    
    // 环境温度范围限制
    if (ambient_temperature < -20.0f) ambient_temperature = -20.0f;
    if (ambient_temperature > 60.0f) ambient_temperature = 60.0f;
    
    // 添加调试输出
    ADC_DEBUG_PRINTF("Ambient Temp: chip=%.2f, filtered=%.2f, ambient=%.2f\n", 
           current_chip_temp, chip_temperature_filtered, ambient_temperature);
}

/**
 * getAmbientTemperatureEstimate - 获取环境温度估计值
 * 
 * 功能：返回基于芯片内部温度估计的环境温度
 * 特点：响应较慢，但能反映环境温度变化趋势
 * 用途：为烙铁头温度补偿提供环境温度参考
 * 
 * @return 估计的环境温度值（°C）
 */
float getAmbientTemperatureEstimate(void) {
    return ambient_temperature;
}