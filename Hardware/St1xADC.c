#include "St1xADC.h"
#include "adc.h"
#include "stdio.h"
#include "u8g2.h"
#include "tim.h"

// 全局变量：ADC采样状态和存储数组
uint8_t adc_sampling_flag = 0;  // 0=停止采样，1=正在采样
uint16_t DMA_ADC[2] = {0};      // 存储两个通道的采样值：[0]烙铁头温度，[1]USB电压

// PWM相关变量（用于控制采样时机）
static uint32_t pwm_falling_edge_time = 0;  // 记录PWM下降沿的时间
static uint8_t waiting_for_delay = 0;       // 是否在等待延迟采样
static uint32_t pwm_period = 10000;         // PWM周期（10kHz频率）

// 按键中断处理函数（当按键按下时自动调用）
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // 模式按键：设置350度并开始加热
    if (GPIO_Pin == KEY_MODE_Pin) {
        setT12Temperature(370.0);  // 设置目标温度
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);  // 启动加热PWM
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000 * 0.5);  // 60%功率加热
    }
    // 上调按键：停止加热
    if (GPIO_Pin == KEY_UP_Pin) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000 * 0);  // 0%功率（停止加热）
        setT12Temperature(0.0);  // 目标温度设为0度
    }
}

// 控制ADC采样时机（避免PWM干扰）
void controlADCSampling(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {  // 只处理TIM2定时器
        // PWM低电平时停止采样（避免干扰）
        if (htim->Instance->CNT < htim->Instance->CCR4) {
            HAL_ADC_Stop_DMA(&hadc1);  // 停止ADC采样
            adc_sampling_flag = 0;     // 标记为停止状态
        } 
        // PWM高电平时开始采样
        else {
            if (adc_sampling_flag == 0) {  // 如果当前没有在采样
                HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);  // 启动ADC采样2个通道
                adc_sampling_flag = 1;  // 标记为采样状态
            }
        }
    }
}

// 数据滤波处理（去除异常值，计算平均值）
void applyMeanFilterAndRemoveOutliers() {
    uint16_t ad1_iron = 0, ad2_usb = 0;  // 滤波后的烙铁头和USB电压值
    int validSamples = 0;  // 有效样本数量
    
    // 检查1000个采样点（实际500对数据）
    for (int i = 0; i < 1000; i += 2) {
        // USB电压通道：超过650的值可能是干扰，忽略
        if (DMA_ADC[1] <= 650) {
            ad2_usb = DMA_ADC[1];  // 累加有效值
            validSamples++;
        }
        // 烙铁头温度通道：同样处理异常值
        if (DMA_ADC[0] <= 650) {
            ad1_iron += DMA_ADC[0];  // 累加有效值
            validSamples++;
        }
    }
    
    // 计算平均值（防止除以0错误）
    if (validSamples > 0) {
        ad1_iron = ad1_iron / (validSamples / 2);  // 烙铁头平均值
        ad2_usb = ad2_usb / (validSamples / 2);    // USB电压平均值
    }
}

// 电压计算和串口输出（用于调试）
void calculateVoltageAndPrint() {
    uint16_t ad1_iron = 0, ad2_usb = 0;
    // 打印USB电压（考虑分压电阻）
    printf("USB=%f\r\nUSB=%d\r\n", ad2_usb * 3.3 / 4095 / 0.151515, DMA_ADC[1]);
    // 打印烙铁头电压
    printf("PEN=%f\r\nPEN=%d\r\n", ad1_iron * 3.3 / 4095, DMA_ADC[0]);
    printf("/*************/\r\n");  // 分隔线
}

// OLED屏幕显示函数（在屏幕上显示各种信息）
void drawOnOLED(u8g2_t *u8g2) {
    // 先处理数据
    controlADCSampling(&htim2);        // 控制采样时机
    applyMeanFilterAndRemoveOutliers(); // 数据滤波
    t12TemperatureControlLoop();        // 温度控制
    
    char usb[500], iron[500];  // 显示文本缓冲区
    u8g2_ClearBuffer(u8g2);    // 清空屏幕缓存

    // 显示大字体温度（32x64像素）
    u8g2_SetFont(u8g2, u8g2_font_spleen32x64_mf);
    sprintf(iron, "%0.0f", (DMA_ADC[0] * 3.3) / (4095 * 0.00281) + 25);  // 温度计算
    u8g2_DrawStr(u8g2, 3, 60, iron);  // 在(3,60)位置显示温度

    // 显示当前温度（小字体）
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    float current_temp = calculateT12Temperature(DMA_ADC[0]);  // 计算实际温度
    sprintf(iron, "CTT:%0.0f", current_temp);  // 当前温度
    u8g2_DrawStr(u8g2, 3, 74, iron);

    // 显示烙铁头电压
    sprintf(iron, "PEN:%0.2fV", DMA_ADC[0] * 3.3 / 4095);
    u8g2_DrawStr(u8g2, 4, 12, iron);

    // 显示USB电压（考虑分压电阻）
    sprintf(usb, "USB:%0.2fV", DMA_ADC[1] * 3.3 / 4095 / 0.151515);
    u8g2_DrawStr(u8g2, 67, 12, usb);

    // 绘制屏幕布局框架
    u8g2_DrawFrame(u8g2, 0, 0, 128, 16);    // 顶部状态栏
    u8g2_DrawFrame(u8g2, 0, 64, 128, 16);   // 底部状态栏
    u8g2_DrawFrame(u8g2, 0, 15, 32, 50);    // 左侧温度框
    u8g2_DrawFrame(u8g2, 31, 15, 97, 50);   // 右侧信息框
    u8g2_DrawLine(u8g2, 64, 0, 64, 15);     // 顶部中间线
    u8g2_DrawLine(u8g2, 64, 64, 64, 80);    // 底部中间线

    // 温度进度条（根据温度值变化）
    u8g2_DrawRBox(u8g2, 64, 64, DMA_ADC[0] / (4095 / 64), 16, 0);  // 填充条
    u8g2_DrawRFrame(u8g2, 64, 64, 64, 16, 0);  // 进度条边框

    u8g2_SendBuffer(u8g2);  // 将缓存内容发送到OLED显示
}

// T12烙铁头温度计算函数
float calculateT12Temperature(uint16_t adcValue) {
    // 温度传感器参数
    const float mV_per_degree = 2.81;     // 热电偶灵敏度（每度2.81mV）
    const float cold_junction_temp = 25.0;   // 环境温度补偿（25度）
    const float adc_ref_voltage = 3.3;      // ADC参考电压（3.3V）
    const uint16_t adc_max = 4095;          // 12位ADC最大值（0-4095）
    
    // 安全检查：ADC值不能超过最大值
    if (adcValue > adc_max) adcValue = adc_max;
    
    // ADC值转电压：ADC读数 × 3.3V ÷ 4095
    float voltage = (adcValue * adc_ref_voltage) / adc_max;
    
    // 电压转温度：电压(mV) ÷ 灵敏度 + 环境温度
    float temperature = (voltage * 1000.0) / mV_per_degree + cold_junction_temp;
    
    // 温度安全限制
    if (temperature < 0) temperature = 0;     // 最低0度
    if (temperature > 400) temperature = 400;  // 最高400度（安全上限）
    
    return temperature;
}

// PID控制器参数（用于精确温度控制）
PID_Controller t12_pid = {1.0, 0.05, 0.2, 0.0, 0.0, 0.0, 0};  // KP=1.0, KI=0.05, KD=0.2

// PID温度控制算法（让温度稳定在目标值）
float pidTemperatureControl(float current_temp) {
    uint32_t current_time = HAL_GetTick();  // 获取当前时间
    float dt = (current_time - t12_pid.last_time) / 1000.0;  // 计算时间间隔（秒）
    
    if (dt <= 0) dt = 0.01;  // 最小时间间隔（防止除0）
    
    // 计算温度误差：目标温度 - 当前温度
    float error = t12_pid.setpoint - current_temp;
    
    // PID三部分计算：
    // P项（比例）：误差 × 比例系数（快速响应）
    float proportional = t12_pid.kp * error;
    
    // I项（积分）：累加误差 × 积分系数（消除稳态误差）
    t12_pid.integral += error * dt;
    // 积分限幅（防止积分过大）
    if (t12_pid.integral > 50) t12_pid.integral = 50;
    if (t12_pid.integral < -50) t12_pid.integral = -50;
    float integral = t12_pid.ki * t12_pid.integral;
    
    // D项（微分）：误差变化率 × 微分系数（抑制振荡）
    float derivative = t12_pid.kd * (error - t12_pid.prev_error) / dt;
    
    // PID输出 = P + I + D
    float output = proportional + integral + derivative;
    
    // 输出限制在0-80%范围内
    if (output > 80.0) output = 80.0;  // 最大100%功率
    if (output < 0.0) output = 0.0;      // 最小0%功率
    
    // 保存当前状态用于下次计算
    t12_pid.prev_error = error;      // 保存当前误差
    t12_pid.last_time = current_time; // 保存当前时间
    
    return output;
}

// 设置目标温度函数
void setT12Temperature(float temperature) {
    t12_pid.setpoint = temperature;  // 设置目标温度
    t12_pid.integral = 0.0;          // 重置积分项（温度变化时重新积累）
    t12_pid.prev_error = 0.0;        // 重置上次误差
    t12_pid.last_time = HAL_GetTick(); // 重置时间戳
}

// 主温度控制循环（不断运行保持温度稳定）
void t12TemperatureControlLoop(void) {
    // 控制ADC采样时机
    controlADCSampling(&htim2);
    
    // 计算当前烙铁头温度
    float current_temp = calculateT12Temperature(DMA_ADC[0]);
    
    // PID计算需要的加热功率（0-100%）
    float pwm_duty_percent = pidTemperatureControl(current_temp);
    
    // 设置PWM占空比（控制加热功率）
    uint16_t pwm_value = (uint16_t)(pwm_duty_percent * 80);  // 转换为0-8000范围
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, pwm_value);
    
    // 过热保护：超过420度自动停止加热
    if (current_temp > 420.0) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);  // 紧急停止加热
    }
}

// 温度校准结构（用于精确校准温度测量）
typedef struct {
    float offset;       // 温度偏移量（校准零点）
    float gain;         // 增益系数（校准斜率）
    uint16_t adc_zero;  // 0度时的ADC读数
    uint16_t adc_span;  // 满量程时的ADC读数
} TemperatureCalibration;

// 默认温度校准参数
TemperatureCalibration t12_cal = {0.0, 1.0, 0, 4095};

// 移动平均滤波配置（让温度显示更平滑）
#define TEMP_FILTER_SIZE 8  // 使用8个历史温度值计算平均值
float temp_history[TEMP_FILTER_SIZE];  // 温度历史数据数组
uint8_t temp_index = 0;                // 当前写入位置

// 移动平均滤波函数（去除温度跳动）
float filteredTemperature(float new_temp) {
    temp_history[temp_index] = new_temp;  // 存储新温度值
    temp_index = (temp_index + 1) % TEMP_FILTER_SIZE;  // 循环写入位置
    
    // 计算8个温度值的平均值
    float sum = 0;
    for (int i = 0; i < TEMP_FILTER_SIZE; i++) {
        sum += temp_history[i];
    }
    return sum / TEMP_FILTER_SIZE;  // 返回平滑后的温度
}