// ====================================================
// 开源地址：https://github.com/leon-red/St1x
// 作者：Leon Red
// 项目启动时间：2023/2/13
// ====================================================
// T12 烙铁控制器 - 温度测量部分（注释已简化、便于初学者理解）
// 这个文件负责处理与温度测量相关的所有功能：
// - 读取温度传感器的数据
// - 把传感器数据转换成我们可以理解的温度值
// - 让温度读数更稳定，不会跳来跳去
// - 检查是否安全（电压够不够，温度会不会太高）
// - 显示相关信息给用户
//
// 小白须知：
// - ADC（模数转换器）就像一个翻译官，把传感器的电压信号翻译成数字(0-4095)
// - 我们再把数字翻译回电压，最后换算成摄氏度
// ====================================================

// ==================== 第一步：包含必要的工具箱 ====================
// 这些头文件就像工具箱，告诉电脑这个程序需要用到哪些功能
#include "St1xADC.h"    // 我们自己写的温度测量功能
#include "St1xPID.h"    // 温度控制功能
#include "adc.h"        // 读取传感器数据的功能
#include "stdio.h"      // 处理文字和数字转换的功能
#include "u8g2.h"       // 控制屏幕显示的功能
#include "tim.h"        // 定时器功能（用来定时做事情）
#include <math.h>       // 数学计算功能（比如求绝对值）

// ==================== 第二步：设置一些常量参数 ====================
// 这些是我们提前设定好的数值，程序运行时不会改变

// 温度传感器的参数（不同传感器可能需要调整）
#define ATemp   0              // 环境温度补偿（修正环境温度对测量的影响）
#define Thermal_Voltage 0.00263f // 热电偶灵敏度（每摄氏度产生的电压）

// 滤波器设置（让温度读数更稳定）
#define TEMP_FILTER_SIZE 4      // 控制用滤波器大小（用4个数平均，更稳定但反应慢一点）
#define DISPLAY_FILTER_SIZE 8   // 显示用滤波器大小（增加滤波器大小让显示更平滑）

// 安全和控制参数
#define USB_VOLTAGE_THRESHOLD 15.0f  // 最低工作电压：低于15伏就不能加热
#define DEBOUNCE_DELAY 50            // 按键防抖时间：防止按键抖动误触发
// 保持与PID文件一致的控制间隔
#define CONTROL_INTERVAL 50          // 控制间隔：每隔50毫秒检查一次温度

// ==================== 第三步：定义全局变量 ====================
// 这些变量用来保存程序运行过程中的各种状态和数据

// 用户设置和运行状态
float target_temperature = 300.0f;  // 目标温度：想要加热到多少度（默认300°C）
uint8_t heating_status = 0;       // 加热状态：0=不加热，1=正在加热

// 传感器数据存储
uint16_t DMA_ADC[2] = {0};        // 传感器读数：存放从传感器读到的原始数据

// 时间和按键相关
uint32_t last_control_time = 0;   // 上次控制时间：记录上次调整温度的时间
uint32_t last_key_press_time = 0; // 上次按键时间：记录上次按按键的时间
uint16_t last_key_pin = 0;        // 按键引脚：记录按的是哪个按键

// 标志位
uint8_t adc_sampling_flag = 0;    // 采样标志：标记是否正在读取传感器数据
uint8_t heating_control_enabled = 0; // 控制标志：标记是否启用了温度控制

static uint8_t first_draw = 1;    // 首次绘制：标记是否第一次显示屏幕内容

// 加热相关时间记录
uint32_t heating_start_time = 0;  // 开始加热时间：记录什么时候开始加热的

// 温度校准参数
TemperatureCalibration t12_cal = {0.0, 1.0, 0, 4095}; // 温度校准参数

// ==================== 第四步：定义内部使用的变量 ====================
// 这些变量只在这个文件内部使用，不会被其他文件访问

// 温度滤波器（用于控制逻辑，让温度更稳定）
static float temperature_buffer[TEMP_FILTER_SIZE] = {0};  // 温度数据缓冲区
static uint8_t filter_index = 0;                          // 当前写入位置
static uint8_t filter_initialized = 0;                    // 是否已初始化
static float filtered_temperature = 0;                    // 滤波后的温度

// 显示用滤波器（用于屏幕显示，反应更平滑）
static float display_temperature_buffer[DISPLAY_FILTER_SIZE] = {0}; // 显示缓冲区
static uint8_t display_filter_index = 0;                            // 显示写入位置
static uint8_t display_filter_initialized = 0;                      // 显示是否初始化
static float display_filtered_temperature = 0;                      // 显示用温度

// 用于显示动画效果的变量
static float displayed_temperature = 0;                    // 实际显示的温度（带动画效果）
static uint32_t last_display_update = 0;                  // 上次显示更新时间

// 控制和PWM相关变量
static uint32_t heating_control_interval = 50;  // 加热控制间隔（与PID文件保持一致）
static uint32_t pwm_falling_edge_time = 0;      // PWM下降沿时间
static uint8_t waiting_for_delay = 0;           // 等待延时标志

// 采样状态机（分步骤进行采样）
uint8_t sampling_phase = 0;              // 采样阶段
uint32_t sample_start_time = 0;          // 阶段开始时间
uint16_t saved_pwm_value = 0;            // 保存的PWM值

// ==================== 第五步：声明内部函数 ====================
// 提前告诉电脑我们会用到哪些函数

// 用于屏幕显示的温度滤波函数
void updateDisplayTemperatureFilter(uint16_t adcValue);
float getDisplayFilteredTemperature(void);

// ==================== 第六步：实现各种功能 ====================

/**
 * calculateT12Temperature - 把传感器读数转换成温度值
 * 
 * 功能：把传感器的数字信号（0-4095）转换成摄氏度温度
 * 原理：
 *  1) 把数字信号转换成电压值
 *  2) 根据传感器特性把电压换算成温度
 *  3) 加上环境温度补偿得到最终温度
 * 安全：如果计算结果不合理，会限制在0-460°C范围内
 * 
 * @param adcValue 传感器读数（0-4095）
 * @return 温度值（摄氏度）
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
    
    // 温度范围限制
    if (temperature < 0) temperature = 0;
    if (temperature > 460) temperature = 460;
    
    return temperature;
}

/**
 * getFilteredTemperature - 获取稳定的温度值
 * 
 * 功能：返回经过滤波处理的稳定温度值
 * 原理：使用多个温度读数的平均值，减少随机波动
 * 
 * @return 稳定的温度值（摄氏度）
 */
float getFilteredTemperature(void) {
    return filtered_temperature;
}

/**
 * updateTemperatureFilter - 更新温度滤波器
 * 
 * 功能：把新的温度读数加入滤波器并计算平均值
 * 原理：维护一个固定大小的缓冲区，循环存储最新的几个温度值并求平均
 * 
 * @param adcValue 新的传感器读数
 */
void updateTemperatureFilter(uint16_t adcValue) {
    // 把传感器读数转换成温度值
    float current_temp = calculateT12Temperature(adcValue);
    
    // 把新温度值存入缓冲区
    temperature_buffer[filter_index] = current_temp;
filter_index = (filter_index + 1) % TEMP_FILTER_SIZE;
    
    // 如果是第一次使用，用当前值填满整个缓冲区
    if (!filter_initialized) {
        for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
            temperature_buffer[i] = current_temp;
        }
filter_initialized = 1;
    }
    
    // 计算平均温度值
    float sum = 0;
    for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
        sum += temperature_buffer[i];
    }
    filtered_temperature = sum / TEMP_FILTER_SIZE;
}

// ==================== 第七步：安全检测功能 ====================

/**
 * isUSBVoltageSufficient - 检查USB电压是否足够
 * 
 * 功能：检查供电电压是否满足加热要求
 * 原理：读取电压传感器数据，换算成实际电压值，判断是否高于阈值
 * 
 * @return 1=电压足够，0=电压不足
 */
uint8_t isUSBVoltageSufficient(void) {
    // 计算实际USB电压值
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.1515f;
    return (usb_voltage >= USB_VOLTAGE_THRESHOLD);
}

/**
 * checkUSBVoltage - 检查并处理USB电压异常
 * 
 * 功能：检测电压异常并自动保护设备
 * 原理：如果电压不足，自动停止加热并关闭温度控制
 * 
 * @return 1=电压正常，0=电压异常
 */
uint8_t checkUSBVoltage(void) {
    // 如果电压不足
    if (!isUSBVoltageSufficient()) {
        // 停止加热
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        return 0;
    }
    return 1;
}

/**
 * checkTemperatureSafety - 检查温度安全
 * 
 * 功能：监控温度是否在安全范围内
 * 原理：检查温度是否超过限制或传感器是否异常
 * 
 * @return 1=温度安全，0=温度异常
 */
uint8_t checkTemperatureSafety(void) {
    float current_temp = getFilteredTemperature();
    
    // 温度过高保护
    if (current_temp > 460.0f) {
        // 停止加热
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        return 0;
    }
    
    // 传感器异常检测
    if (current_temp < 0.0f || current_temp > 500.0f) {
        // 停止加热
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        return 0;
    }
    
    return 1;
}

// ==================== 第八步：ADC采样控制 ====================

/**
 * controlADCSampling - 控制ADC采样时机
 * 
 * 功能：选择合适的时间读取传感器数据
 * 原理：避免在加热器工作时读取数据，减少干扰
 * 
 * @param htim 定时器句柄
 */
void controlADCSampling(TIM_HandleTypeDef *htim) {
    // 这个函数由定时器中断调用，不包含阻塞延时
    // 采样逻辑在定时器回调中处理，避免阻塞系统
    // PWM状态由调用者控制
}

// ==================== 第九步：用户交互功能 ====================

/**
 * HAL_GPIO_EXTI_Callback - 按键中断处理函数
 * 
 * 功能：处理用户按键操作
 * 原理：根据按下的按键执行相应操作（开始/停止加热，调整温度）
 * 
 * @param GPIO_Pin 按键对应的引脚
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t current_time = HAL_GetTick();
    
    // 按键防抖：避免按键抖动引起的误操作
    if ((current_time - last_key_press_time) < DEBOUNCE_DELAY) {
        return;
    }
    
    last_key_press_time = current_time;
    last_key_pin = GPIO_Pin;
    
    // 模式键（开始/停止加热）
    if (GPIO_Pin == KEY_MODE_Pin) {
        // 先检查电压是否足够
        if (!isUSBVoltageSufficient()) {
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
            stopHeatingControlTimer();
            heating_status = 0;
            return;
        }
        
        // 如果当前没有加热
        if (heating_status == 0) {
            // 开始加热
            setT12Temperature(target_temperature);
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
            
            // 记录开始加热时间
            heating_start_time = current_time;
            
            // 设置初始功率为100%
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
            
            startHeatingControlTimer();
            heating_status = 1;
            
            // 重置采样状态机
            sampling_phase = 0;
            sample_start_time = 0;
            saved_pwm_value = 0;
            
            // 初始化显示温度
            float current_temp = getFilteredTemperature();
            displayed_temperature = current_temp;
        } else {
            // 停止加热
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
            setT12Temperature(target_temperature); // 重新设置目标温度以重置PID状态
            stopHeatingControlTimer();
            heating_status = 0;
            
            sampling_phase = 0;
            sample_start_time = 0;
            saved_pwm_value = 0;
        }
    }
    // 温度增加键
    else if (GPIO_Pin == KEY_UP_Pin) {
        // 增加5度
        target_temperature += 5.0;
        
        // 温度上限保护
        if (target_temperature > 460.0) {
            target_temperature = 460.0;
        }
        
        // 如果正在加热，更新目标温度
        if (heating_status == 1) {
            setT12Temperature(target_temperature);
        }
    }
    // 温度减少键
    else if (GPIO_Pin == KEY_DOWN_Pin) {
        // 减少5度
        target_temperature -= 5.0;
        
        // 温度下限保护
        if (target_temperature < 0.0) {
            target_temperature = 0.0;
        }
        
        // 如果正在加热，更新目标温度
        if (heating_status == 1) {
            setT12Temperature(target_temperature);
        }
    }
}

/**
 * drawOnOLED - 在OLED屏幕上显示信息
 * 
 * 功能：在屏幕上显示温度、电压等信息
 * 原理：使用u8g2库在OLED屏幕上绘制各种信息
 * 
 * @param u8g2 OLED显示对象
 */
void drawOnOLED(u8g2_t *u8g2) {
    char display_buffer[32];
    
    // 确保传感器数据正在更新
    if (adc_sampling_flag == 0) {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);
    }

    // 获取要显示的数据
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.152f;

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
    if (last_display_update == 0) {
        last_display_update = current_time;
        displayed_temperature = raw_temp;
    }
    
    // 计算时间差（秒）
    float time_delta = (current_time - last_display_update) / 1000.0f;
    last_display_update = current_time;
    
    // 限制时间差，防止异常值
    if (time_delta > 0.5f) time_delta = 0.1f;
    
    // 使用指数平滑算法实现更丝滑的温度显示
    // 平滑因子根据温度变化速度动态调整
    float temp_diff = fabs(filtered_temp - displayed_temperature);
    float smoothing_factor = 0.1f; // 基础平滑因子
    
    // 温度变化越大，平滑因子越小（变化越慢），让显示更稳定
    if (temp_diff > 10.0f) {
        smoothing_factor = 0.02f;
    } else if (temp_diff > 5.0f) {
        smoothing_factor = 0.05f;
    } else if (temp_diff > 2.0f) {
        smoothing_factor = 0.08f;
    }
    
    // 更新显示温度
    displayed_temperature = displayed_temperature + smoothing_factor * (filtered_temp - displayed_temperature);
    
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
    
    // 显示加热状态（当处于专注加热模式时显示）
    extern uint8_t focused_heating_mode;
    if (heating_status) {
        if (focused_heating_mode) {
            u8g2_DrawStr(u8g2, 83, 62, "Heating");  // 显示加热状态
        }
    } else {
        // 当加热停止时显示"Stop"
        u8g2_DrawStr(u8g2, 102, 62, "Stop");
    }
    
    // 绘制界面边框
    u8g2_DrawFrame(u8g2, 0, 0, 128, 16);
    u8g2_DrawFrame(u8g2, 0, 64, 128, 16);
    u8g2_DrawFrame(u8g2, 0, 15, 32, 50);
    u8g2_DrawFrame(u8g2, 31, 15, 97, 50);
    u8g2_DrawLine(u8g2, 64, 0, 64, 15);
    u8g2_DrawLine(u8g2, 64, 64, 64, 80);

    // 把缓冲区内容发送到屏幕显示
    u8g2_SendBuffer(u8g2);
}

// ==================== 第十步：显示专用功能 ====================

/**
 * updateDisplayTemperatureFilter - 更新显示用温度滤波器
 * 
 * 功能：为屏幕显示提供更平滑的温度读数
 * 原理：使用较大的滤波窗口，让显示响应更平滑
 * 
 * @param adcValue 传感器读数
 */
void updateDisplayTemperatureFilter(uint16_t adcValue) {
    // 防止传感器读数异常
    if (adcValue > 4000) {
        return;
    }
    
    float current_temp = calculateT12Temperature(adcValue);
    
    display_temperature_buffer[display_filter_index] = current_temp;
    display_filter_index = (display_filter_index + 1) % DISPLAY_FILTER_SIZE;
    
    // 如果是第一次使用，初始化缓冲区
    if (!display_filter_initialized) {
        for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
            display_temperature_buffer[i] = current_temp;
        }
        display_filter_initialized = 1;
    }
    
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
 * 功能：返回用于屏幕显示的温度值
 * 
 * @return 显示用温度值
 */
float getDisplayFilteredTemperature(void) {
    return display_filtered_temperature;
}