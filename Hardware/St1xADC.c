// ====================================================
// T12烙铁控制器 - 温度测量和控制系统
// 功能：测量烙铁头温度、控制加热功率、显示信息
// 适合初学者学习的嵌入式系统示例
// ====================================================

#include "St1xADC.h"
#include "adc.h"
#include "stdio.h"
#include "u8g2.h"
#include "tim.h"

// ==================== 所有全局变量定义（按依赖关系排列） ====================

// 温度传感器校准参数（必须在最前面，因为其他函数依赖这些参数）
// ATemp: 环境温度补偿（比如室温25度）
// Thermal_Voltage: 热电偶灵敏度（每度温度变化对应的电压变化）
double ATemp = 0;
double Thermal_Voltage = 0.0041;

// 默认温度校准参数（未校准状态）
TemperatureCalibration t12_cal = {0.0, 1.0, 0, 4095};

// 温度移动平均滤波相关变量（必须在滤波函数之前定义）
#define TEMP_FILTER_SIZE 4  // 滤波器窗口大小（4点移动平均）
static float temperature_buffer[TEMP_FILTER_SIZE] = {0};  // 温度数据缓冲区
static uint8_t filter_index = 0;                          // 当前缓冲区索引
static uint8_t filter_initialized = 0;                     // 滤波器初始化标志
static float filtered_temperature = 0;                    // 滤波后的温度值

// 加热控制状态变量（必须在定时器函数之前定义）
static uint8_t heating_control_enabled = 0;  // 0=禁用自动控制，1=启用自动控制
static uint32_t heating_control_interval = 100;  // 控制间隔100毫秒

// PWM控制相关变量（防止PWM干扰温度测量）
static uint32_t pwm_falling_edge_time = 0;  // 记录PWM下降的时间
static uint8_t waiting_for_delay = 0;       // 等待延迟采样的标志
static uint32_t pwm_period = 10000;         // PWM周期（10kHz频率）

// 按键防抖相关变量
static uint32_t last_key_press_time = 0;    // 上次按键时间戳（毫秒）
static uint16_t last_key_pin = 0;           // 上次按下的按键引脚
static const uint32_t DEBOUNCE_DELAY = 50;  // 防抖延时50毫秒

// 核心控制变量（用户设置和系统状态）
float target_temperature = 330;  // 目标温度设置 - 用户想要达到的温度（默认330度）
uint8_t heating_status = 0;      // 加热状态标志：0=停止加热，1=正在加热

// ADC采样相关变量
uint16_t DMA_ADC[2] = {0};       // 存储ADC采样数据：数组第一个是烙铁头温度，第二个是USB电压
uint8_t adc_sampling_flag = 0;   // 这个标志告诉程序ADC是否正在采样：0=停止，1=正在采样

// PID控制器参数 - 这是温度控制的核心参数
// KP=比例系数（响应速度），KI=积分系数（消除误差），KD=微分系数（抑制振荡）
PID_Controller t12_pid = {0.8, 0.02, 0.5, 0.0, 0.0, 0.0, 0};

// ==================== 核心功能：温度计算和PID控制 ====================

/**
 * 把ADC采样值转换成实际温度值
 * 输入：ADC采样值（0-4095之间的数字）
 * 输出：实际的摄氏度温度
 * 原理：ADC值 → 电压值 → 温度值
 */
float calculateT12Temperature(uint16_t adcValue) {
    // 传感器参数（这些值需要根据实际传感器校准）
    const float mV_per_degree = Thermal_Voltage;  // 热电偶灵敏度（每度对应的电压变化）
    const float cold_junction_temp = ATemp;       // 环境温度补偿（比如室温25度）
    const float adc_ref_voltage = 3.3;           // ADC参考电压（STM32是3.3V）
    const uint16_t adc_max = 4095;               // 12位ADC的最大值（2^12-1=4095）
    
    // 安全检查：确保ADC值不会超过最大值
    if (adcValue > adc_max) adcValue = adc_max;
    
    // 第一步：ADC值转成电压（ADC读数 × 3.3V ÷ 4095）
    float voltage = (adcValue * adc_ref_voltage) / adc_max;
    
    // 第二步：电压转成温度（电压 ÷ 灵敏度 + 环境温度）
    float temperature = voltage / mV_per_degree + cold_junction_temp;
    
    // 温度安全限制：防止温度过高或过低
    if (temperature < 0) temperature = 0;     // 最低0度（不能低于0度）
    if (temperature > 460) temperature = 460; // 最高460度（安全上限）
    
    return temperature;
}

/**
 * PID温度控制算法 - 让温度稳定在目标值
 * 作用：根据当前温度和目标温度的差异，计算合适的加热功率
 * 原理：P（比例）+ I（积分）+ D（微分）= 合适的加热功率
 */
float pidTemperatureControl(float current_temp) {
    uint32_t current_time = HAL_GetTick();  // 获取当前时间（毫秒）
    float dt = (current_time - t12_pid.last_time) / 1000.0;  // 计算时间间隔（秒）
    
    if (dt <= 0) dt = 0.01;  // 防止时间间隔为0导致计算错误
    
    // 计算温度误差：目标温度 - 当前温度
    float error = t12_pid.setpoint - current_temp;
    
    // PID三部分计算：
    // P项（比例控制）：误差越大，加热功率越大（快速响应）
    float proportional = t12_pid.kp * error;
    
    // I项（积分控制）：累计误差，消除稳态误差（比如始终差几度）
    t12_pid.integral += error * dt;
    // 积分限幅：防止积分过大导致温度过冲
    if (t12_pid.integral > 20) t12_pid.integral = 20;    // 减小积分限幅
    if (t12_pid.integral < -20) t12_pid.integral = -20;  // 减小积分限幅
    float integral = t12_pid.ki * t12_pid.integral;
    
    // D项（微分控制）：根据温度变化速度调整，抑制振荡
    float derivative = t12_pid.kd * (error - t12_pid.prev_error) / dt;
    
    // PID输出 = 比例项 + 积分项 + 微分项
    float output = proportional + integral + derivative;
    
    // 输出功率限制在0-100%范围内
    if (output > 95.0) output = 95.0;  // 最大95%功率（留有余量防止过冲）
    if (output < 0.0) output = 0.0;      // 最小0%功率（停止加热）
    
    // 保存状态用于下次计算
    t12_pid.prev_error = error;      // 保存当前误差
    t12_pid.last_time = current_time; // 保存当前时间
    
    return output;
}

/**
 * 设置目标温度
 * 当用户改变目标温度时，需要重置PID控制器的状态
 * 作用：防止温度突变时PID控制器反应过度
 */
void setT12Temperature(float temperature) {
    t12_pid.setpoint = temperature;  // 设置新的目标温度
    t12_pid.integral = 0.0;           // 重置积分项（重新开始积累误差）
    t12_pid.prev_error = 0.0;         // 重置上次误差
    t12_pid.last_time = HAL_GetTick(); // 重置时间戳
}

// ==================== 数据处理功能：移动平均滤波 ====================

/**
 * 更新温度移动平均滤波器
 * 输入：新的ADC采样值
 * 作用：将新温度值加入滤波器，计算移动平均值
 * 原理：用最近4个温度值的平均值作为当前温度，减少噪声干扰
 */
void updateTemperatureFilter(uint16_t adcValue) {
    // 计算当前温度
    float current_temp = calculateT12Temperature(adcValue);
    
    // 将新温度值加入缓冲区
    temperature_buffer[filter_index] = current_temp;
    filter_index = (filter_index + 1) % TEMP_FILTER_SIZE;
    
    // 如果滤波器未初始化，用当前温度填充整个缓冲区
    if (!filter_initialized) {
        for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
            temperature_buffer[i] = current_temp;
        }
        filter_initialized = 1;
    }
    
    // 计算移动平均值
    float sum = 0;
    for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
        sum += temperature_buffer[i];
    }
    filtered_temperature = sum / TEMP_FILTER_SIZE;
}

/**
 * 获取滤波后的温度值
 * 输出：经过移动平均滤波的温度值
 * 作用：提供稳定的温度读数，减少跳动
 */
float getFilteredTemperature(void) {
    return filtered_temperature;
}

// ==================== 核心功能：温度控制定时器 ====================

/**
 * 启动加热控制定时器
 * 启用后，系统会每隔一段时间自动调整加热功率
 * 作用：实现自动温度控制，让温度稳定在目标值
 */
void startHeatingControlTimer(void) {
    heating_control_enabled = 1;
    HAL_TIM_Base_Start_IT(&htim2);  // 启动TIM2定时器中断
}

/**
 * 停止加热控制定时器
 * 停止自动温度控制，加热功率保持不变
 * 作用：手动控制加热或停止加热时使用
 */
void stopHeatingControlTimer(void) {
    heating_control_enabled = 0;
    HAL_TIM_Base_Stop_IT(&htim2);  // 停止TIM2定时器中断
}

/**
 * 定时器中断回调函数 - 自动温度控制逻辑
 * 这个函数由定时器硬件自动调用，每隔一段时间执行一次
 * 这是整个温度控制系统的核心逻辑
 */
void heatingControlTimerCallback(void) {
    if (!heating_control_enabled) return;  // 如果自动控制被禁用，直接返回
    
    static uint32_t last_control_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 检查是否达到控制间隔
    if ((current_time - last_control_time) < heating_control_interval) {
        return;  // 未达到间隔时间，跳过本次控制
    }
    
    last_control_time = current_time;
    
    // 1. 控制ADC采样时机，确保温度测量准确
    controlADCSampling(&htim2);
    
    // 2. 更新温度滤波器并获取滤波后的温度
    updateTemperatureFilter(DMA_ADC[0]);
    float current_temp = getFilteredTemperature();
    
    // 3. 使用PID算法计算合适的加热功率（0-100%）
    float pwm_duty_percent = pidTemperatureControl(current_temp);
    
    // 4. 设置PWM占空比来控制加热功率
    uint16_t pwm_value = (uint16_t)(pwm_duty_percent * 10000 / 100);
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, pwm_value);
    
    // 5. 过热保护：如果温度超过460度，自动停止加热
    if (current_temp > 460.0) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);  // 紧急停止加热
        stopHeatingControlTimer();  // 停止自动温度控制
    }
}

// ==================== 辅助功能：ADC采样控制 ====================

/**
 * 控制ADC采样时机 - 避免PWM加热信号干扰温度测量
 * 原理：在PWM高电平（加热时）采样，低电平（不加热时）停止采样
 * 作用：提高温度测量精度，避免PWM干扰
 */
void controlADCSampling(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {  // 只处理加热用的TIM2定时器
        
        // PWM低电平时（不加热）：停止ADC采样，避免干扰
        if (htim->Instance->CNT < htim->Instance->CCR4) {
            HAL_ADC_Stop_DMA(&hadc1);  // 停止ADC采样
            adc_sampling_flag = 0;      // 更新状态为"停止采样"
        } 
        // PWM高电平时（加热）：开始ADC采样
        else {
            if (adc_sampling_flag == 0) {  // 如果当前没有在采样
                HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);  // 启动两个通道的采样
                adc_sampling_flag = 1;      // 更新状态为"正在采样"
            }
        }
    }
}

// ==================== 辅助功能：用户交互 ====================

/**
 * 按键中断处理函数 - 当用户按下按键时自动执行
 * 这个函数由STM32硬件自动调用，不需要手动调用
 * 添加了按键防抖功能，防止按键抖动导致的误触发
 * KEY_MODE_Pin按键功能：按一下开始加热，再按一下停止加热
 * KEY_UP_Pin按键功能：按一下增加5度目标温度
 * KEY_DOWN_Pin按键功能：按一下减少5度目标温度
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t current_time = HAL_GetTick();  // 获取当前时间（毫秒）
    
    // 按键防抖检查：如果两次按键间隔太短，认为是抖动，忽略这次按键
    if ((current_time - last_key_press_time) < DEBOUNCE_DELAY) {
        return;  // 防抖：忽略短时间内重复的按键
    }
    
    // 记录这次按键的时间和引脚
    last_key_press_time = current_time;
    last_key_pin = GPIO_Pin;
    
    // 如果按下的是"模式"按键（切换加热状态）
    if (GPIO_Pin == KEY_MODE_Pin) {
        // 切换加热状态
        if (heating_status == 0) {
            // 当前是停止状态，开始加热
            setT12Temperature(target_temperature);          // 设置目标温度
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);   // 启动PWM加热
            startHeatingControlTimer();          // 开始温度自动控制
            heating_status = 1;                  // 更新状态为"正在加热"
        } else {
            // 当前是加热状态，停止加热
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);  // 停止加热（功率设为0%）
            setT12Temperature(0.0);              // 目标温度设为0度
            stopHeatingControlTimer();           // 停止温度自动控制
            heating_status = 0;                  // 更新状态为"停止加热"
        }
    }
    // 如果按下的是"增加温度"按键
    else if (GPIO_Pin == KEY_UP_Pin) {
        // 增加目标温度5度
        target_temperature += 5.0;
        
        // 温度安全限制：最高不超过460度（安全上限）
        if (target_temperature > 460.0) {
            target_temperature = 460.0;
        }
        
        // 如果当前正在加热，更新PID控制器的目标温度
        if (heating_status == 1) {
            setT12Temperature(target_temperature);
        }
    }
    // 如果按下的是"减少温度"按键
    else if (GPIO_Pin == KEY_DOWN_Pin) {
        // 减少目标温度5度
        target_temperature -= 5.0;
        
        // 温度安全限制：最低不低于0度
        if (target_temperature < 0.0) {
            target_temperature = 0.0;
        }
        
        // 如果当前正在加热，更新PID控制器的目标温度
        if (heating_status == 1) {
            setT12Temperature(target_temperature);
        }
    }
}

/**
 * 在OLED屏幕上显示温度、电压等信息
 * 这个函数负责把数据可视化显示给用户看
 * 作用：让用户了解当前系统状态
 */
void drawOnOLED(u8g2_t *u8g2) {
    char display_buffer[32];  // 用于存储显示文字的缓冲区
    
    // 1. 先控制ADC采样时机，确保数据准确
    controlADCSampling(&htim2);
    
    // 2. 清空屏幕缓存，准备绘制新内容
    u8g2_ClearBuffer(u8g2);
    
    // 3. 获取滤波后的温度和USB电压
    updateTemperatureFilter(DMA_ADC[0]);  // 更新滤波器
    float current_temp = getFilteredTemperature();  // 获取滤波后的温度
    float usb_voltage = DMA_ADC[1] * 3.3 / 4095 / 0.1515f;  // USB电压（未滤波）
    
    // 4. 显示大字体温度（屏幕中间的大数字）
//    u8g2_SetFont(u8g2, u8g2_font_spleen32x64_me);
    u8g2_SetFont(u8g2, u8g2_font_fur30_tf);
    sprintf(display_buffer, "C %0.0f", current_temp);
    u8g2_DrawStr(u8g2, 2, 56, display_buffer);
    
    // 5. 显示小字体当前温度
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    sprintf(display_buffer, "CTT:%0.0f", current_temp);
    u8g2_DrawStr(u8g2, 3, 76, display_buffer);
    
    // 6. 显示烙铁头原始电压值
    sprintf(display_buffer, "PEN:%d", DMA_ADC[0]);
    u8g2_DrawStr(u8g2, 4, 12, display_buffer);
    
    // 7. 显示USB电压（经过分压电阻计算后的实际电压）
    sprintf(display_buffer, "USB:%0.2fV", usb_voltage);
    u8g2_DrawStr(u8g2, 67, 12, display_buffer);
    // 8. 显示目标温度
    sprintf(display_buffer, "TEMP:%0.0f", target_temperature);
    u8g2_DrawStr(u8g2, 66, 76, display_buffer);
    
    // 8. 绘制屏幕边框和分割线，让界面更美观
    u8g2_DrawFrame(u8g2, 0, 0, 128, 16);    // 顶部状态栏边框
    u8g2_DrawFrame(u8g2, 0, 64, 128, 16);   // 底部状态栏边框
    u8g2_DrawFrame(u8g2, 0, 15, 32, 50);    // 左侧温度框
    u8g2_DrawFrame(u8g2, 31, 15, 97, 50);   // 右侧信息框
    u8g2_DrawLine(u8g2, 64, 0, 64, 15);     // 顶部中间分割线
    u8g2_DrawLine(u8g2, 64, 64, 64, 80);     // 底部中间分割线

    // 9. 把绘制好的内容发送到OLED屏幕显示
    u8g2_SendBuffer(u8g2);
}