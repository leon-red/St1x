// ====================================================
// T12烙铁控制器 - 温度测量和控制系统
// 功能：测量烙铁头温度、控制加热功率、显示信息
// 适合初学者学习的嵌入式系统示例
// 开源地址：https://github.com/leon-red/St1x
// 作者：Leon Red
// 项目启动时间：2023/2/13.
// ====================================================

// ==================== 第一步：包含必要的头文件 ====================
// 这些头文件就像是工具箱，告诉编译器我们需要使用哪些功能
#include "St1xADC.h"    // 项目自己的头文件，包含自定义的结构体定义
#include "adc.h"        // ADC（模数转换器）相关功能
#include "stdio.h"      // 标准输入输出，用于格式化字符串显示
#include "u8g2.h"       // OLED屏幕显示库
#include "tim.h"        // 定时器相关功能
#include <math.h>       // 数学函数库，支持fabs等函数

// ==================== 第二步：系统常量定义（最重要的设置） ====================
// 这些常量就像是系统的"身份证"，定义了整个系统的基本参数

// 温度传感器校准参数（需要根据实际传感器调整）
#define ATemp   0;              // 环境温度补偿，比如室温25度
#define Thermal_Voltage 0.0041f; // 热电偶灵敏度，每度对应的电压变化

// 滤波器窗口大小（用于平滑温度读数）
#define TEMP_FILTER_SIZE 4      // PID温度滤波器窗口大小（4个采样点）
#define DISPLAY_FILTER_SIZE 2   // 显示温度滤波器窗口大小（2个采样点，响应更快）

// 安全保护参数（保护系统和用户安全）
#define USB_VOLTAGE_THRESHOLD 15.0f  // USB电压阈值，低于15V时禁止加热（防止电压不足）
#define DEBOUNCE_DELAY 50            // 按键防抖延时50毫秒（防止按键抖动误触发）
#define PWM_PERIOD 10000             // PWM周期（10kHz频率，控制加热功率）

// ==================== 第三步：全局变量定义（系统核心状态） ====================
// 这些变量记录系统的当前状态，就像汽车的仪表盘

// 用户设置的核心参数
float target_temperature = 300;      // 目标温度设置（默认300度）
uint8_t heating_status = 0;          // 加热状态：0=停止加热，1=正在加热

// ADC采样相关变量（温度测量的"眼睛"）
uint16_t DMA_ADC[2] = {0};           // 存储ADC采样数据（通道0：温度，通道1：电压）
uint8_t adc_sampling_flag = 0;       // ADC采样状态：0=停止采样，1=正在采样

// PID控制器参数（温度控制的"大脑"）
PID_Controller t12_pid = {0.5, 0.01, 1.2, 0.0, 0.0, 0.0, 0};
// 参数说明：KP=0.5（比例系数），KI=0.01（积分系数），KD=1.2（微分系数）

// 温度校准参数（确保温度测量准确）
TemperatureCalibration t12_cal = {0.0, 1.0, 0, 4095};

// ==================== 第四步：静态变量定义（内部使用的变量） ====================
// 这些变量只在当前文件内部使用，不对外暴露

// 温度移动平均滤波相关变量（让温度读数更稳定）
static float temperature_buffer[TEMP_FILTER_SIZE] = {0};  // 温度缓冲区
static uint8_t filter_index = 0;                          // 当前缓冲区位置
static uint8_t filter_initialized = 0;                    // 滤波器初始化标志
static float filtered_temperature = 0;                     // 滤波后的温度值

// 显示专用温度滤波器变量（让屏幕显示更流畅）
static float display_temperature_buffer[DISPLAY_FILTER_SIZE] = {0};
static uint8_t display_filter_index = 0;
static uint8_t display_filter_initialized = 0;
static float display_filtered_temperature = 0;

// 加热控制状态变量（控制系统的"心跳"）
static uint8_t heating_control_enabled = 0;      // 0=禁用自动控制，1=启用自动控制
static uint32_t heating_control_interval = 100;  // 控制间隔100毫秒（10次/秒）

// PWM控制相关变量（精确控制加热功率）
static uint32_t pwm_falling_edge_time = 0;      // 记录PWM下降的时间
static uint8_t waiting_for_delay = 0;           // 等待延迟采样的标志

// 按键防抖相关变量（防止误操作）
static uint32_t last_key_press_time = 0;        // 上次按键时间戳
static uint16_t last_key_pin = 0;               // 上次按下的按键引脚

// ==================== 第五步：函数声明（告诉编译器有哪些函数） ====================
// 就像书的目录，先告诉编译器后面会定义这些函数

// 显示专用滤波器函数声明
void updateDisplayTemperatureFilter(uint16_t adcValue);
float getDisplayFilteredTemperature(void);

// ==================== 第六步：核心功能实现（最重要的部分） ====================

/**
 * 【核心功能1】把ADC采样值转换成实际温度值
 * 作用：将ADC读取的数字信号转换成我们能理解的温度值
 * 原理：ADC值 → 电压值 → 温度值（三步转换）
 * 输入：ADC采样值（0-4095之间的数字）
 * 输出：实际的摄氏度温度
 */
float calculateT12Temperature(uint16_t adcValue) {
    // 传感器参数（这些值需要根据实际传感器校准）
    const float mV_per_degree = Thermal_Voltage;  // 热电偶灵敏度（每度对应的电压变化）
    const float cold_junction_temp = ATemp;       // 环境温度补偿（比如室温25度）
    const float adc_ref_voltage = 3.3;           // ADC参考电压（STM32是3.3V）
    const uint16_t adc_max = 4095;               // 12位ADC的最大值（2^12-1=4095）
    
    // 安全检查：确保ADC值不会超过最大值（防止异常情况）
    if (adcValue > adc_max) adcValue = adc_max;
    
    // 第一步：ADC值转成电压（ADC读数 × 3.3V ÷ 4095）
    // 就像把数字信号转换成实际的电压值
    float voltage = (adcValue * adc_ref_voltage) / adc_max;
    
    // 第二步：电压转成温度（电压 ÷ 灵敏度 + 环境温度）
    // 根据热电偶特性，将电压值转换成温度值
    float temperature = voltage / mV_per_degree + cold_junction_temp;
    
    // 温度安全限制：防止温度过高或过低（保护系统和用户）
    if (temperature < 0) temperature = 0;     // 最低0度（不能低于0度）
    if (temperature > 460) temperature = 460; // 最高460度（安全上限）
    
    return temperature;
}

/**
 * 【核心功能2】PID温度控制算法 - 让温度稳定在目标值
 * 作用：根据当前温度和目标温度的差异，计算合适的加热功率
 * 原理：P（比例）+ I（积分）+ D（微分）= 合适的加热功率
 * 就像开车时根据距离目的地的远近调整油门大小
 */
float pidTemperatureControl(float current_temp) {
    uint32_t current_time = HAL_GetTick();  // 获取当前时间（毫秒）
    float dt = (current_time - t12_pid.last_time) / 1000.0;  // 计算时间间隔（秒）
    
    if (dt <= 0) dt = 0.01;  // 防止时间间隔为0导致计算错误
    
    // 计算温度误差：目标温度 - 当前温度（还差多少度）
    float error = t12_pid.setpoint - current_temp;
    
    // PID三部分计算（就像三个助手一起工作）：
    
    // P项（比例控制）：误差越大，加热功率越大（快速响应）
    // 就像看到目标还很远，就猛踩油门
    float proportional = t12_pid.kp * error;
    
    // I项（积分控制）：累计误差，消除稳态误差（比如始终差几度）
    // 就像发现总是差一点，就慢慢补上
    t12_pid.integral += error * dt;
    
    // 【重要优化】积分衰减：当温度接近目标温度时（误差小于5度），对积分项进行衰减
    // 防止在目标温度附近积分项过大导致温度过冲
    float integral_decay_factor = 1.0f;
    if (fabs(error) < 5.0f) {
        // 误差越小，积分衰减越强（防止在目标温度附近积分项过大）
        integral_decay_factor = fabs(error) / 5.0f;
        if (integral_decay_factor < 0.1f) integral_decay_factor = 0.1f;  // 最小衰减到10%
    }
    t12_pid.integral *= integral_decay_factor;
    
    // 积分限幅：进一步减小积分限幅，防止积分过大导致温度过冲
    if (t12_pid.integral > 10) t12_pid.integral = 10;    // 减小积分限幅
    if (t12_pid.integral < -10) t12_pid.integral = -10;  // 减小积分限幅
    float integral = t12_pid.ki * t12_pid.integral;
    
    // D项（微分控制）：根据温度变化速度调整，抑制振荡
    // 就像看到温度变化太快，就适当刹车防止冲过头
    float derivative = t12_pid.kd * (error - t12_pid.prev_error) / dt;
    
    // PID输出 = 比例项 + 积分项 + 微分项（三个助手的结果加起来）
    float output = proportional + integral + derivative;
    
    // 输出功率限制在0-95%范围内（安全保护）
    if (output > 95.0) output = 95.0;  // 最大95%功率（留有余量防止过冲）
    if (output < 0.0) output = 0.0;      // 最小0%功率（停止加热）
    
    // 保存状态用于下次计算（记住这次的情况）
    t12_pid.prev_error = error;      // 保存当前误差
    t12_pid.last_time = current_time; // 保存当前时间
    
    return output;
}

/**
 * 【核心功能3】设置目标温度
 * 作用：当用户改变目标温度时，需要重置PID控制器的状态
 * 原理：防止温度突变时PID控制器反应过度
 * 就像换了一个目的地，要重新规划路线
 */
void setT12Temperature(float temperature) {
    t12_pid.setpoint = temperature;  // 设置新的目标温度
    t12_pid.integral = 0.0;           // 重置积分项（重新开始积累误差）
    t12_pid.prev_error = 0.0;         // 重置上次误差
    t12_pid.last_time = HAL_GetTick(); // 重置时间戳
}

// ==================== 第七步：数据处理功能（让数据更准确） ====================

/**
 * 【数据处理1】更新温度移动平均滤波器
 * 作用：将新温度值加入滤波器，计算移动平均值
 * 原理：用最近4个温度值的平均值作为当前温度，减少噪声干扰
 * 就像取多次测量的平均值，让结果更稳定
 */
void updateTemperatureFilter(uint16_t adcValue) {
    // 计算当前温度
    float current_temp = calculateT12Temperature(adcValue);
    
    // 将新温度值加入缓冲区（循环使用4个位置）
    temperature_buffer[filter_index] = current_temp;
    filter_index = (filter_index + 1) % TEMP_FILTER_SIZE;
    
    // 如果滤波器未初始化，用当前温度填充整个缓冲区
    if (!filter_initialized) {
        for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
            temperature_buffer[i] = current_temp;
        }
        filter_initialized = 1;
    }
    
    // 计算移动平均值（4个温度值的平均数）
    float sum = 0;
    for (uint8_t i = 0; i < TEMP_FILTER_SIZE; i++) {
        sum += temperature_buffer[i];
    }
    filtered_temperature = sum / TEMP_FILTER_SIZE;
}

/**
 * 【数据处理2】获取滤波后的温度值
 * 作用：提供稳定的温度读数，减少跳动
 * 输出：经过移动平均滤波的温度值
 */
float getFilteredTemperature(void) {
    return filtered_temperature;
}

// ==================== 第八步：定时器控制功能（系统的心跳） ====================

/**
 * 【定时器控制1】启动加热控制定时器
 * 作用：启用后，系统会每隔一段时间自动调整加热功率
 * 原理：实现自动温度控制，让温度稳定在目标值
 */
void startHeatingControlTimer(void) {
    heating_control_enabled = 1;
    HAL_TIM_Base_Start_IT(&htim2);  // 启动TIM2定时器中断
}

/**
 * 【定时器控制2】停止加热控制定时器
 * 作用：停止自动温度控制，加热功率保持不变
 * 原理：手动控制加热或停止加热时使用
 */
void stopHeatingControlTimer(void) {
    heating_control_enabled = 0;
    HAL_TIM_Base_Stop_IT(&htim2);  // 停止TIM2定时器中断
}

/**
 * 【安全检测】检查USB电压是否足够（高于15V）
 * 作用：防止电压不足时加热，保护系统安全
 * 输出：1=电压足够可以加热，0=电压不足禁止加热
 */
uint8_t isUSBVoltageSufficient(void) {
    // 计算USB电压（ADC值转电压，考虑分压电阻）
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.1515f;
    return (usb_voltage >= USB_VOLTAGE_THRESHOLD);
}

// ==================== 第九步：核心控制逻辑（系统的大脑） ====================

/**
 * 【核心控制】定时器中断回调函数 - 自动温度控制逻辑
 * 作用：这个函数由定时器硬件自动调用，每隔一段时间执行一次
 * 原理：这是整个温度控制系统的核心逻辑，就像系统的心跳
 */
void heatingControlTimerCallback(void) {
    if (!heating_control_enabled) return;  // 如果自动控制被禁用，直接返回
    
    static uint32_t last_control_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 检查是否达到控制间隔（100毫秒执行一次）
    if ((current_time - last_control_time) < heating_control_interval) {
        return;  // 未达到间隔时间，跳过本次控制
    }
    
    last_control_time = current_time;
    
    // 1. 控制ADC采样时机，确保温度测量准确
    controlADCSampling(&htim2);
    
    // 2. 检查USB电压是否足够，如果不足则停止加热（安全第一）
    if (!isUSBVoltageSufficient()) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);  // 紧急停止加热
        stopHeatingControlTimer();  // 停止自动温度控制
        heating_status = 0;         // 更新状态为"停止加热"
        return;  // 直接返回，不执行后续控制逻辑
    }
    
    // 3. 更新温度滤波器并获取滤波后的温度
    updateTemperatureFilter(DMA_ADC[0]);
    float current_temp = getFilteredTemperature();
    
    // 4. 使用PID算法计算合适的加热功率（0-100%）
    float pwm_duty_percent = pidTemperatureControl(current_temp);
    
    // 5. 设置PWM占空比来控制加热功率
    uint16_t pwm_value = (uint16_t)(pwm_duty_percent * PWM_PERIOD / 100);
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, pwm_value);
    
    // 6. 过热保护：如果温度超过460度，自动停止加热（安全保护）
    if (current_temp > 460.0) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);  // 紧急停止加热
        stopHeatingControlTimer();  // 停止自动温度控制
    }
}

// ==================== 第十步：ADC采样控制（精确测量） ====================

/**
 * 【ADC控制】控制ADC采样时机 - 避免PWM加热信号干扰温度测量
 * 作用：提高温度测量精度，避免PWM干扰
 * 原理：在PWM高电平（加热时）采样，低电平（不加热时）停止采样
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

// ==================== 第十一步：用户交互功能（人机界面） ====================

/**
 * 【用户交互1】按键中断处理函数 - 当用户按下按键时自动执行
 * 作用：处理用户的按键操作，实现温度设置和加热控制
 * 原理：这个函数由STM32硬件自动调用，不需要手动调用
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
        // 检查USB电压是否足够
        if (!isUSBVoltageSufficient()) {
            // USB电压低于15V，禁止加热（安全保护）
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);  // 确保停止加热
            stopHeatingControlTimer();           // 停止温度自动控制
            heating_status = 0;                  // 更新状态为"停止加热"
            return;  // 直接返回，不执行加热操作
        }
        
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
 * 【用户交互2】在OLED屏幕上显示温度、电压等信息
 * 作用：将系统状态信息显示给用户
 * 原理：使用u8g2库在OLED屏幕上绘制各种信息
 */
void drawOnOLED(u8g2_t *u8g2) {
    char display_buffer[32];
    
    // 1. 确保ADC采样正在进行
    if (adc_sampling_flag == 0) {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);
        HAL_Delay(1);
    }
    
    // 2. 更新显示专用温度滤波器
    updateDisplayTemperatureFilter(DMA_ADC[0]);
    
    // 3. 清空屏幕缓存
    u8g2_ClearBuffer(u8g2);
    
    // 4. 获取显示数据 - 分开获取不同用途的温度
    float raw_temp = getDisplayFilteredTemperature();  // 原始温度（显示专用）
    float pid_temp = getFilteredTemperature();         // PID实际温度
    float usb_voltage = DMA_ADC[1] * 3.3f / 4095.0f / 0.1515f;  // USB电压
    
    // 5. 显示内容 - 按照需求分开显示
    u8g2_SetFont(u8g2, u8g2_font_fur30_tf);
    sprintf(display_buffer, "C %0.0f", raw_temp);  // 显示原始温度
    u8g2_DrawStr(u8g2, 2, 56, display_buffer);
    
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    sprintf(display_buffer, "PID:%0.0f", pid_temp);  // 显示PID温度
    u8g2_DrawStr(u8g2, 3, 76, display_buffer);
    
    sprintf(display_buffer, "PEN:%d", DMA_ADC[0]);
    u8g2_DrawStr(u8g2, 4, 12, display_buffer);
    
    sprintf(display_buffer, "USB:%0.2fV", usb_voltage);  // 显示USB电压
    u8g2_DrawStr(u8g2, 67, 12, display_buffer);
    
    if (usb_voltage >= USB_VOLTAGE_THRESHOLD) {
        u8g2_DrawStr(u8g2, 114, 25, "OK");
    } else {
        u8g2_DrawStr(u8g2, 108, 25, "LOW");
    }
    
    sprintf(display_buffer, "TEMP:%0.0f", target_temperature);
    u8g2_DrawStr(u8g2, 66, 76, display_buffer);
    
    // 绘制界面框架
    u8g2_DrawFrame(u8g2, 0, 0, 128, 16);
    u8g2_DrawFrame(u8g2, 0, 64, 128, 16);
    u8g2_DrawFrame(u8g2, 0, 15, 32, 50);
    u8g2_DrawFrame(u8g2, 31, 15, 97, 50);
    u8g2_DrawLine(u8g2, 64, 0, 64, 15);
    u8g2_DrawLine(u8g2, 64, 64, 64, 80);

    // 发送到屏幕
    u8g2_SendBuffer(u8g2);
}

// ==================== 第十二步：显示专用功能（优化显示效果） ====================

/**
 * 【显示优化1】更新显示专用温度滤波器
 * 作用：为屏幕显示提供更流畅的温度读数
 * 原理：使用较小的滤波器窗口，让显示响应更快
 */
void updateDisplayTemperatureFilter(uint16_t adcValue) {
    // 量程检查：如果ADC值超过4095（量程上限），不更新滤波器
    if (adcValue > 4095) {
        return;  // 超过量程，直接返回，不更新温度值
    }
    
    float current_temp = calculateT12Temperature(adcValue);
    
    display_temperature_buffer[display_filter_index] = current_temp;
    display_filter_index = (display_filter_index + 1) % DISPLAY_FILTER_SIZE;
    
    if (!display_filter_initialized) {
        for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
            display_temperature_buffer[i] = current_temp;
        }
        display_filter_initialized = 1;
    }
    
    float sum = 0;
    for (uint8_t i = 0; i < DISPLAY_FILTER_SIZE; i++) {
        sum += display_temperature_buffer[i];
    }
    display_filtered_temperature = sum / DISPLAY_FILTER_SIZE;
}

/**
 * 【显示优化2】获取显示用滤波温度
 * 作用：提供专门用于显示的温度值
 * 输出：经过显示专用滤波器处理的温度值
 */
float getDisplayFilteredTemperature(void) {
    return display_filtered_temperature;
}