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
#include "stdio.h"      // 标准输入输出，用于调试打印
#include "u8g2.h"       // OLED显示驱动库
#include "tim.h"        // 定时器驱动
#include <math.h>       // 数学函数库

// ==================== 模块二：系统常量配置 ====================

// 温度传感器参数配置
#define ATemp   0              // 环境温度补偿值（冷端补偿）
#define Thermal_Voltage 0.0033f // 热电偶电压-温度转换系数（mV/°C）

// 滤波器配置
#define TEMP_FILTER_SIZE 4      // 控制用温度滤波器窗口大小（快速响应）
#define DISPLAY_FILTER_SIZE 8   // 显示用温度滤波器窗口大小（平滑显示）

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
uint16_t DMA_ADC[2] = {0};           // DMA传输的ADC原始数据（通道0:温度，通道1:电压）

// 时间记录变量
uint32_t last_control_time = 0;      // 上次PID控制执行时间
uint32_t last_key_press_time = 0;    // 上次按键按下时间
uint16_t last_key_pin = 0;           // 上次按下的按键引脚

// 系统状态标志
uint8_t adc_sampling_flag = 0;       // ADC采样状态标志
uint8_t heating_control_enabled = 0; // PID控制使能标志
static uint8_t first_draw = 1;        // 首次屏幕绘制标志

// 加热过程记录
uint32_t heating_start_time = 0;     // 加热开始时间戳

// 温度校准参数结构
TemperatureCalibration t12_cal = {0.0, 1.0, 0, 4095}; // T12烙铁头校准参数

// ==================== 模块四：内部私有变量 ====================

// 控制用温度滤波器（快速响应，用于PID控制）
static float temperature_buffer[TEMP_FILTER_SIZE] = {0};  // 温度数据环形缓冲区
static uint8_t filter_index = 0;                          // 缓冲区写入索引
static uint8_t filter_initialized = 0;                    // 滤波器初始化标志
static float filtered_temperature = 0;                   // 滤波后的控制温度

// 显示用温度滤波器（平滑显示，用于OLED）
static float display_temperature_buffer[DISPLAY_FILTER_SIZE] = {0}; // 显示温度缓冲区
static uint8_t display_filter_index = 0;                             // 显示缓冲区索引
static uint8_t display_filter_initialized = 0;                      // 显示滤波器初始化标志
static float display_filtered_temperature = 0;                      // 滤波后的显示温度

// 温度显示动画系统
static float displayed_temperature = 0;                    // 当前屏幕显示的温度值
static uint32_t last_display_update = 0;                  // 上次显示更新时间
static uint8_t first_display_update = 1;                  // 首次显示更新标志

// PWM加热控制相关
static uint32_t heating_control_interval = 50;  // 加热控制周期
static uint32_t pwm_falling_edge_time = 0;      // PWM下降沿时间记录
static uint8_t waiting_for_delay = 0;           // 延时等待标志

// ADC采样状态机
uint8_t sampling_phase = 0;              // 当前采样阶段
uint32_t sample_start_time = 0;          // 采样阶段开始时间
uint16_t saved_pwm_value = 0;            // 采样时保存的PWM值

// ==================== 模块五：内部函数声明 ====================

// 温度显示平滑函数
float smoothTemperatureDisplay(float current_display, float target_temp, float time_delta);

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
    const float adc_ref_voltage = 3.3;            // ADC参考电压
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
    const float adc_ref_voltage = 3.3;            // ADC参考电压
    const uint16_t adc_max = 4095;                // ADC最大值
    
    // 防止读数异常
    if (adcValue > adc_max) adcValue = adc_max;
    
    // 数字信号转电压值
    float voltage = (adcValue * adc_ref_voltage) / adc_max;
    
    // 电压值转温度值
    float temperature = voltage / mV_per_degree + cold_junction_temp;
    
    // 添加调试：检查150°C和250°C对应的ADC值
    if (fabs(temperature - 150.0f) < 1.0f) {
        float expected_adc_150 = calculateADCForTemperature(150.0f);
        ADC_DEBUG_PRINTF("DEBUG 150°C: Temp=%.2f, ADC=%d, Voltage=%.4f, Expected ADC=%.1f\n", 
               temperature, adcValue, voltage, expected_adc_150);
    }
    if (fabs(temperature - 250.0f) < 1.0f) {
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
    if (fabs(current_temp - 150.0f) < 5.0f || fabs(current_temp - 250.0f) < 5.0f) {
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
    if (fabs(new_filtered_temp - 150.0f) < 5.0f || fabs(new_filtered_temp - 250.0f) < 5.0f) {
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

// ==================== 模块九：ADC采样控制 ====================

/**
 * controlADCSampling - ADC采样时机控制
 * 
 * 功能：管理ADC采样时机，避免加热干扰
 * 原理：在定时器中断中调用，确保采样与PWM同步
 * 特点：非阻塞设计，采样逻辑在回调中处理
 * 
 * @param htim 定时器句柄
 */
void controlADCSampling(TIM_HandleTypeDef *htim) {
    // 此函数由定时器中断调用，不包含阻塞延时
    // 实际采样逻辑在定时器回调中处理，避免阻塞系统
    // PWM状态由调用者控制，确保采样与加热同步
}

// ==================== 模块十：OLED显示系统 ====================

/**
 * HAL_GPIO_EXTI_Callback - 外部中断回调函数（保留）
 * 
 * 功能：保留的外部中断处理函数
 * 现状：按键处理已移至St1xKey模块，此处为空实现
 * 用途：防止编译错误，保持代码兼容性
 * 
 * @param GPIO_Pin 中断引脚编号
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // 按键处理已移至St1xKey模块，此处不进行任何操作
    // 保留此函数以防止编译错误
    (void)GPIO_Pin; // 避免未使用参数警告
}

/**
 * drawOnOLED - OLED屏幕信息显示主函数
 * 
 * 功能：在OLED屏幕上实时显示温度、电压、状态等信息
 * 显示内容：当前温度、目标温度、PID温度、ADC值、USB电压、加热状态
 * 重启检测：使用系统启动时间和PID状态双重判断重启状态
 * 温度显示：重启后直接显示实际温度，正常启动使用平滑动画
 * 
 * @param u8g2 OLED显示对象指针
 */
void drawOnOLED(u8g2_t *u8g2) {
    char display_buffer[32];
    
    // 确保传感器数据正在更新
    if (adc_sampling_flag == 0) {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);
    }

    // 获取要显示的数据
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.151f;

    // 直接使用传感器数据计算显示温度
    float raw_temp = calculateT12Temperature(DMA_ADC[0]);

    // 更新显示用滤波器
    updateDisplayTemperatureFilter(DMA_ADC[0]);
    // 使用滤波后的显示温度
    float filtered_temp = getDisplayFilteredTemperature();
    
    // 获取控制用的滤波温度
    float pid_temp = getFilteredTemperature();
    
    // 实现平滑的温度显示动画效果
    uint32_t current_time = HAL_GetTick();
    
    // 强制高频更新显示，消除停顿感
    // 每次调用都更新显示，确保流畅性
    
    // 判断是否重启后首次显示：使用系统启动时间和PID状态双重判断
    static uint32_t system_start_time = 0;
    if (system_start_time == 0) {
        system_start_time = current_time;  // 记录系统启动时间
    }
    
    // 重启检测：系统启动时间较短（<2秒）且PID未工作
    if ((current_time - system_start_time) < 2000 && !heating_control_enabled) {
        // 重启后首次显示更新，直接使用当前实际温度，不使用平滑算法
        last_display_update = current_time;
        displayed_temperature = filtered_temp;
        
        // 添加调试信息
        ADC_DEBUG_PRINTF("[RESTART] System startup detected, PID not working, display initialized with actual temperature: %.1f°C\n", displayed_temperature);
    } else if (first_display_update) {
        // 正常启动后的首次显示，使用平滑算法
        float time_delta = (current_time - last_display_update) / 1000.0f;
        last_display_update = current_time;
        
        if (time_delta > 0.05f) time_delta = 0.02f;
        
        displayed_temperature = smoothTemperatureDisplay(displayed_temperature, filtered_temp, time_delta);
        first_display_update = 0;  // 清除首次显示标志
    } else {
        // 正常显示更新，使用平滑算法
        // 计算时间差（秒），大幅提高更新频率
        float time_delta = (current_time - last_display_update) / 1000.0f;
        last_display_update = current_time;
        
        // 大幅提高更新频率，适应快速升温
        if (time_delta > 0.05f) time_delta = 0.02f; // 降低上限到20ms，确保高频更新
        
        // 调用平滑显示函数
        displayed_temperature = smoothTemperatureDisplay(displayed_temperature, filtered_temp, time_delta);
    }
    
    // 添加调试：检查显示温度计算
    if (fabs(displayed_temperature - 150.0f) < 2.0f || fabs(displayed_temperature - 250.0f) < 2.0f) {
        float temp_diff_debug = filtered_temp - displayed_temperature;
        ADC_DEBUG_PRINTF("DISPLAY: raw=%.2f, filtered=%.2f, displayed=%.2f, diff=%.2f\n", 
               raw_temp, filtered_temp, displayed_temperature, temp_diff_debug);
    }
    
    // 清空屏幕缓冲区
    u8g2_ClearBuffer(u8g2);
    
    // 显示当前温度（大字体）
    u8g2_SetFont(u8g2, u8g2_font_fur30_tf);
//    u8g2_SetFontPosTop(u8g2);
    sprintf(display_buffer, "%0.0f", displayed_temperature);
    u8g2_DrawStr(u8g2, 33, 56, display_buffer);
    u8g2_DrawStr(u8g2, 3, 56, "C");
    
    // 显示其他信息（小字体）
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    
    // 显示控制用温度
    sprintf(display_buffer, "PID:%0.0f", pid_temp);
    u8g2_DrawStr(u8g2, 3, 76, display_buffer);
    
    // 显示传感器原始数据
    sprintf(display_buffer, "ADC0:%d", DMA_ADC[0]);
    u8g2_DrawStr(u8g2, 4, 12, display_buffer);
    
    // 显示USB电压
    sprintf(display_buffer, "USB:%0.1f V", usb_voltage);
    u8g2_DrawStr(u8g2, 67, 12, display_buffer);
    
    // 显示电压状态
    if (usb_voltage >= USB_VOLTAGE_THRESHOLD) {
        u8g2_DrawStr(u8g2, 114, 26, "OK");
    } else {
        u8g2_DrawStr(u8g2, 108, 26, "LOW");
    }
    
    // 显示目标温度
    sprintf(display_buffer, "SET:%0.0f", target_temperature);
    u8g2_DrawStr(u8g2, 68, 76, display_buffer);
    
    // 显示加热状态
    extern uint8_t focused_heating_mode;
    extern uint8_t heating_control_enabled;
    
    // 更友好的状态显示逻辑
    if (!heating_control_enabled) {
        // 1. PID是否工作-否-不显示
        // 不显示任何状态文字
    } else if (!heating_status) {
        // 2. PID是否工作-是-加热状态-否-显示"Stop"
        u8g2_DrawStr(u8g2, 102, 62, "Stop");
    } else if (focused_heating_mode) {
        // 3. PID是否工作-是-是否进入专注模式-是-显示"Heating"
        u8g2_DrawStr(u8g2, 83, 62, "Heating");
    } else {
        // 4. PID是否工作-是-是否进入专注模式-否-PID是否在控制状态-是-显示"Work"
        u8g2_DrawStr(u8g2, 102, 62, "Work");
    }
/*

    // 绘制界面边框
    // 绘制屏幕外边框
    u8g2_DrawFrame(u8g2, 0, 0, 128, 80);
// 绘制内部水平分隔线
    u8g2_DrawLine(u8g2, 0, 15, 127, 15);   // 中间顶部水平线
    u8g2_DrawLine(u8g2, 0, 64, 127, 64);   // 中间底部水平线
// 绘制内部垂直分隔线
    u8g2_DrawLine(u8g2, 31, 15, 31, 64);   // 左侧面板右侧垂直线
    u8g2_DrawLine(u8g2, 64, 0, 64, 15);    // 顶部中间垂直线
    u8g2_DrawLine(u8g2, 64, 64, 64, 80);
*/

    // 把缓冲区内容发送到屏幕显示
    u8g2_SendBuffer(u8g2);
}

// ==================== 模块十一：显示温度滤波系统 ====================

/**
 * updateDisplayTemperatureFilter - 更新显示用温度滤波器
 * 
 * 功能：为OLED显示提供平滑的温度读数
 * 原理：使用8点移动平均滤波，提供稳定的显示效果
 * 特点：大窗口滤波，响应平滑，适合视觉显示
 * 重启处理：保持first_display_update标志，支持重启检测
 * 
 * @param adcValue ADC原始读数
 */
void updateDisplayTemperatureFilter(uint16_t adcValue) {
    // 防止传感器读数异常
    if (adcValue > 4000) {
        return;
    }
    
    float current_temp = calculateT12Temperature(adcValue);
    
    // 如果是第一次使用，初始化缓冲区和显示温度
    if (!display_filter_initialized) {
        for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
            display_temperature_buffer[i] = current_temp;
        }
        display_filtered_temperature = current_temp;
        
        // 只在重启后首次初始化时设置显示温度，但保持first_display_update标志不变
        // 这样drawOnOLED函数中的重启检测逻辑才能正常工作
        if (first_display_update) {
            displayed_temperature = current_temp;  // 初始化显示温度
        }
        
        display_filter_initialized = 1;
    }
    
    display_temperature_buffer[display_filter_index] = current_temp;
    display_filter_index = (display_filter_index + 1) % DISPLAY_FILTER_SIZE;
    
    // 计算平均值
    float sum = 0;
    for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
        sum += display_temperature_buffer[i];
    }
    display_filtered_temperature = sum / DISPLAY_FILTER_SIZE;
}

/**
 * getDisplayFilteredTemperature - 获取显示用滤波温度
 * 
 * 功能：返回经过8点滤波处理的显示温度值
 * 特点：响应平滑，适合OLED屏幕显示
 * 
 * @return 显示用滤波温度值（°C）
 */
float getDisplayFilteredTemperature(void) {
    return display_filtered_temperature;
}

// ==================== 模块十二：系统状态监控 ====================

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
        last_status_check = current_time;
    }
}

// ==================== 模块十三：温度显示动画系统 ====================

/**
 * smoothTemperatureDisplay - 温度显示平滑动画函数
 * 
 * 功能：实现无停顿的连续温度显示动画效果
 * 原理：基于动态速度因子的智能插值算法
 * 特点：温差越大速度越快，温差越小越精确
 * 速度控制：适应8-12秒快速升温需求
 * 安全限制：限制最大单次变化幅度
 * 
 * @param current_display 当前显示温度（°C）
 * @param target_temp 目标实际温度（°C）
 * @param time_delta 时间差（秒）
 * @return 平滑后的显示温度（°C）
 */
float smoothTemperatureDisplay(float current_display, float target_temp, float time_delta) {
    // 优化版平滑算法：7秒内完成温度显示
    // 基于动态加速和智能插值的快速响应算法
    float temp_diff = target_temp - current_display;
    float abs_temp_diff = fabs(temp_diff);
    
    // 动态速度因子：大幅提升响应速度，实现7秒内显示
    float speed_factor = 0.0f;
    
    // 优化速度控制：适应7秒内快速显示
    if (abs_temp_diff > 100.0f) {
        speed_factor = 120.0f; // 超大温差极速响应
    } else if (abs_temp_diff > 50.0f) {
        speed_factor = 80.0f;  // 大温差快速响应
    } else if (abs_temp_diff > 30.0f) {
        speed_factor = 60.0f;  // 中等温差快速响应
    } else if (abs_temp_diff > 15.0f) {
        speed_factor = 40.0f;  // 小温差快速响应
    } else if (abs_temp_diff > 8.0f) {
        speed_factor = 25.0f;  // 接近目标快速响应
    } else if (abs_temp_diff > 3.0f) {
        speed_factor = 15.0f;  // 精确接近阶段
    } else if (abs_temp_diff > 1.0f) {
        speed_factor = 8.0f;   // 微调阶段
    } else {
        speed_factor = 4.0f;   // 最终稳定阶段
    }
    
    // 根据温度阶段智能加速
    if (current_display < 150.0f) {
        speed_factor *= 1.8f; // 低温阶段大幅加速
    } else if (current_display > 300.0f) {
        speed_factor *= 0.9f; // 高温阶段轻微减速
    }
    
    // 计算目标步长（度/秒 * 时间差）
    float target_step = speed_factor * time_delta;
    
    // 确保步长方向正确
    if (temp_diff < 0) {
        target_step = -target_step;
    }
    
    // 放宽最大单次变化幅度限制
    float max_step = 15.0f; // 放宽限制以支持更快响应
    if (fabs(target_step) > max_step) {
        target_step = (target_step > 0) ? max_step : -max_step;
    }
    
    // 确保显示温度不会超过实际温度
    if (fabs(temp_diff) < fabs(target_step)) {
        // 如果步长会超过目标温度，直接到达目标
        return target_temp;
    } else {
        // 否则正常更新
        float new_display = current_display + target_step;
        
        // 强制最小变化，确保显示持续更新
        if (abs_temp_diff > 0.5f && fabs(target_step) < 0.5f) {
            new_display += (temp_diff > 0) ? 0.5f : -0.5f;
        }
        
        return new_display;
    }
}