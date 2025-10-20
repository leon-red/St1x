#include "St1xADC.h"
#include "adc.h"
#include "stdio.h"
#include "u8g2.h"
#include "tim.h"


uint8_t adc_sampling_flag = 0;
uint16_t DMA_ADC[2] = {0};

// 添加延迟采样相关变量
static uint32_t pwm_falling_edge_time = 0;  // PWM下降沿时间戳
static uint8_t waiting_for_delay = 0;       // 等待延迟标志
static uint32_t pwm_period = 10000;         // PWM周期（假设为10000）

// PWM 信号控制 ADC 采样
void controlADCSampling(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {  // 检查是否为 TIM2
        if (htim->Instance->CNT < htim->Instance->CCR4) {  // 检查当前计数器值是否小于通道4的占空比值
            HAL_ADC_Stop_DMA(&hadc1);  // 停止 ADC DMA 传输
            adc_sampling_flag = 0;  // 设置采样标志为0
        } else {
            if (adc_sampling_flag == 0) {  // 如果采样标志为0
                HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);  // 启动 ADC DMA 传输
                adc_sampling_flag = 1;  // 设置采样标志为1
            }
        }
    }
}

void applyMeanFilterAndRemoveOutliers() {
    uint16_t ad1_iron = 0, ad2_usb = 0;
    int validSamples = 0;
    // 计算总和并记录有效样本数
    for (int i = 0; i < 1000; i += 2) {
        if (DMA_ADC[0] <= 650) {  // 限制在4050以下的值
            ad2_usb = DMA_ADC[0];
            validSamples++;
        }
        if (DMA_ADC[1] <= 650) {
            ad1_iron += DMA_ADC[1];
            validSamples++;
        }
    }
    // 检查有效样本数，避免除以0的情况
    if (validSamples > 0) {
        ad1_iron = ad1_iron / (validSamples / 2);  // 相当于除以有效样本数的一半
        ad2_usb = ad2_usb / (validSamples / 2);    // 相当于除以有效样本数的一半
    } else {
        // 处理无效样本数的情况，可以给出警告或采取其他操作
    }
}

// 计算电压并输出
void calculateVoltageAndPrint() {
    uint16_t ad1_iron = 0, ad2_usb = 0;
printf("USB=%f\r\nUSB=%d\r\n", ad2_usb * 3.3 / 4096 / 0.151515, DMA_ADC[0]);
printf("PEN=%f\r\nPEN=%d\r\n", ad1_iron * 3.3 / 4096, DMA_ADC[1]);
printf("/*************/\r\n");
//    HAL_Delay(100);
}

// 在 OLED 屏幕上绘制信息
void drawOnOLED(u8g2_t *u8g2) {
    controlADCSampling(&htim2);  // 控制 ADC 采样
    char usb[500], iron[500];
    u8g2_ClearBuffer(u8g2);

    // 绘制烙铁头电压
//    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);    //小字体调试使用
    u8g2_SetFont(u8g2, u8g2_font_spleen32x64_mf); //大字体
    sprintf(iron, " %0.0f", (DMA_ADC[0] * 3.3) / (4096 * 0.00256) + 25);
//    u8g2_DrawStr(u8g2, 35, 46, iron);    //调试用
    u8g2_DrawStr(u8g2, 3, 60, iron);

    // 绘制烙铁头电压信息
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    sprintf(iron, "PEN:%0.2fV", DMA_ADC[0] * 3.3 / 4096);
    u8g2_DrawStr(u8g2, 4, 12, iron);

    // 绘制 USB 电压信息
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    sprintf(usb, "USB:%0.2fV", DMA_ADC[1] * 3.3 / 4096 / 0.151515);
    u8g2_DrawStr(u8g2, 67, 12, usb);

    //屏幕布局线框
    u8g2_DrawFrame(u8g2, 0, 0, 128, 16);
    u8g2_DrawFrame(u8g2, 0, 64, 128, 16);
    u8g2_DrawFrame(u8g2,0,15,32,50);
    u8g2_DrawFrame(u8g2,31,15,97,50);
    u8g2_DrawLine(u8g2, 64, 0, 64, 15);
    u8g2_DrawLine(u8g2, 64, 64, 64, 80);

    u8g2_DrawRBox(u8g2, 64, 64, DMA_ADC[0] / (4096 / 64), 16, 0);//圆角填充框矩形框
    u8g2_DrawRFrame(u8g2, 64, 64, 64, 16, 0);//圆角矩形

    u8g2_SendBuffer(u8g2);
//    Test_ws2812();
    // 设置 TIM2 通道4的 PWM 占空比

//    HAL_Delay(10);
}

// 控制烙铁头电流输出
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == KEY_MODE_Pin) {
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
                __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000 * 0.5);
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 1000 * 0.3);
    }
    if (GPIO_Pin == KEY_UP_Pin) {
                __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000 * 0);
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 1000 * 0);
    }
}

// 添加T12温度计算函数
// 改进的温度计算函数（添加边界检查和校准）
float calculateT12Temperature(uint16_t adcValue) {
    // 参数定义
    const float mV_per_degree = 0.041; // T12热电偶灵敏度 mV/°C
    const float cold_junction_temp = 25.0; // 冷端温度
    const float adc_ref_voltage = 3.3; // ADC参考电压
    const uint16_t adc_max = 4095; // 12位ADC最大值
    
    // ADC值边界检查
    if (adcValue > adc_max) adcValue = adc_max;
    
    // 计算电压值
    float voltage = (adcValue * adc_ref_voltage) / adc_max;
    
    // 热电偶电压转换为温度（简化线性模型）
    float temperature = (voltage * 1000.0) / mV_per_degree + cold_junction_temp;
    
    // 温度边界检查
    if (temperature < 0) temperature = 0;
    if (temperature > 500) temperature = 500; // 最大温度限制
    
    return temperature;
}

// 改进的ADC采样控制函数 - 支持PWM下降沿后延迟30%采样
void controlT12ADCSampling(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        uint32_t current_time = HAL_GetTick();
        
        // 检测PWM下降沿（从高电平到低电平的转换）
        static uint8_t last_pwm_state = 0;
        uint8_t current_pwm_state = (htim->Instance->CNT < htim->Instance->CCR4) ? 1 : 0;
        
        // 检测下降沿（从1到0的转换）
        if (last_pwm_state == 1 && current_pwm_state == 0) {
            // 记录下降沿时间
            pwm_falling_edge_time = current_time;
            waiting_for_delay = 1;
            
            // 停止ADC采样，等待延迟
            HAL_ADC_Stop_DMA(&hadc1);
            adc_sampling_flag = 0;
        }
        
        last_pwm_state = current_pwm_state;
        
        // 处理延迟采样逻辑
        if (waiting_for_delay) {
            // 计算延迟时间（PWM周期的30%）
            uint32_t delay_time = pwm_period * 0.3;
            
            // 检查是否达到延迟时间
            if ((current_time - pwm_falling_edge_time) >= delay_time) {
                // 延迟时间已到，开始ADC采样
                if (adc_sampling_flag == 0) {
                    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);
                    adc_sampling_flag = 1;
                    waiting_for_delay = 0;  // 清除等待标志
                }
            }
        } else if (current_pwm_state == 1) {
            // PWM高电平期间（加热中）- 停止ADC采样
            HAL_ADC_Stop_DMA(&hadc1);
            adc_sampling_flag = 0;
            waiting_for_delay = 0;  // 清除等待标志
        }
        // 如果既不在等待延迟 Also not in high voltage, then keep current sampling status
    }
}


PID_Controller t12_pid = {2.0, 0.1, 0.5, 300.0, 0.0, 0.0, 0};

// PID温度控制函数
float pidTemperatureControl(float current_temp) {
    uint32_t current_time = HAL_GetTick();
    float dt = (current_time - t12_pid.last_time) / 1000.0; // 转换为秒
    
    if (dt <= 0) dt = 0.01; // 最小时间间隔
    
    float error = t12_pid.setpoint - current_temp;
    
    // 比例项
    float proportional = t12_pid.kp * error;
    
    // 积分项（抗积分饱和）
    t12_pid.integral += error * dt;
    // 积分限幅
    if (t12_pid.integral > 100) t12_pid.integral = 100;
    if (t12_pid.integral < -100) t12_pid.integral = -100;
    float integral = t12_pid.ki * t12_pid.integral;
    
    // 微分项
    float derivative = t12_pid.kd * (error - t12_pid.prev_error) / dt;
    
    // PID输出
    float output = proportional + integral + derivative;
    
    // 输出限幅 0-100%
    if (output > 100.0) output = 100.0;
    if (output < 0.0) output = 0.0;
    
    t12_pid.prev_error = error;
    t12_pid.last_time = current_time;
    
    return output;
}

// 设置目标温度
void setT12Temperature(float temperature) {
    t12_pid.setpoint = temperature;
    t12_pid.integral = 0.0; // 重置积分项
    t12_pid.prev_error = 0.0;
}

// 修复的主温度控制函数
void t12TemperatureControlLoop(void) {
    // 控制ADC采样时机
    controlT12ADCSampling(&htim2);
    
    // 计算当前温度（假设DMA_ADC[1]是T12温度传感器）
    float current_temp = calculateT12Temperature(DMA_ADC[1]);
    
    // PID计算控制量（0-100%）
    float pwm_duty_percent = pidTemperatureControl(current_temp);
    
    // 设置PWM占空比（假设PWM周期为10000）
    uint16_t pwm_value = (uint16_t)(pwm_duty_percent * 100); // 0-100%转换为0-10000
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, pwm_value);
    
    // 温度保护（超过400度自动关闭加热）
    if (current_temp > 400.0) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 0);
    }
}

// 温度校准结构体
typedef struct {
    float offset;      // 温度偏移
    float gain;        // 增益系数
    uint16_t adc_zero; // 零点ADC值
    uint16_t adc_span; // 量程ADC值
} TemperatureCalibration;

TemperatureCalibration t12_cal = {0.0, 1.0, 0, 4095};

// 移动平均滤波
#define TEMP_FILTER_SIZE 8
float temp_history[TEMP_FILTER_SIZE];
uint8_t temp_index = 0;

float filteredTemperature(float new_temp) {
    temp_history[temp_index] = new_temp;
    temp_index = (temp_index + 1) % TEMP_FILTER_SIZE;
    
    float sum = 0;
    for (int i = 0; i < TEMP_FILTER_SIZE; i++) {
        sum += temp_history[i];
    }
    return sum / TEMP_FILTER_SIZE;
}