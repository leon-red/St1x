//
// T12烙铁控制器 - 头文件声明
// 这个文件包含了所有函数的声明，告诉编译器有哪些函数可用
//

#ifndef ST1XADC_H
#define ST1XADC_H
#include "u8g2.h"
#include "stm32f1xx_hal.h"

// ==================== 核心数据结构定义 ====================

// PID控制器结构体定义 - 这是温度控制的核心算法
typedef struct {
    float kp;           // 比例系数（控制响应速度，越大响应越快）
    float ki;           // 积分系数（消除稳态误差，越大消除误差越快）
    float kd;           // 微分系数（抑制振荡，越大抑制效果越好）
    float setpoint;     // 目标温度（用户设定的温度值）
    float integral;     // 积分项（累计误差，用于消除温度偏差）
    float prev_error;   // 上一次的误差值（用于计算温度变化速度）
    uint32_t last_time; // 上一次计算的时间（用于计算时间间隔）
} PID_Controller;

// 温度校准结构（用于校准温度测量的准确性）
typedef struct {
    float offset;       // 温度偏移量（校准零点误差，比如室温补偿）
    float gain;         // 增益系数（校准斜率误差，调整温度灵敏度）
    uint16_t adc_zero;  // 0度时的ADC读数（用于校准零点）
    uint16_t adc_span;  // 满量程时的ADC读数（用于校准量程）
} TemperatureCalibration;

// ==================== 全局变量声明 ====================

// 全局PID控制器实例 - 这是温度控制的核心
extern PID_Controller t12_pid;

// 加热状态标志：0=停止加热，1=正在加热
extern uint8_t heating_status;

// 全局OLED显示对象 - 用于显示温度信息给用户
extern u8g2_t u8g2;

// ==================== 核心功能函数声明 ====================

// 温度计算和转换函数 - 这是系统最核心的功能
float calculateT12Temperature(uint16_t adcValue);  // ADC值转温度

// PID温度控制函数 - 让温度稳定在目标值
float pidTemperatureControl(float current_temp);  // PID控制算法
void setT12Temperature(float temperature);        // 设置目标温度

// 定时器中断加热控制函数 - 自动温度控制的核心
void startHeatingControlTimer(void);    // 启动自动温度控制
void stopHeatingControlTimer(void);     // 停止自动温度控制
void heatingControlTimerCallback(void); // 定时器中断处理函数

// ==================== 数据处理功能函数声明 ====================

// 移动平均滤波函数 - 让温度读数更稳定
// 在现有声明后添加
float getFilteredTemperature(void);  // 确保这个函数有声明

void updateTemperatureFilter(uint16_t adcValue);  // 更新温度滤波器

// ==================== 辅助功能函数声明 ====================

// PWM信号控制ADC采样（避免干扰）- 提高温度测量精度
void controlADCSampling(TIM_HandleTypeDef *htim);

// OLED屏幕显示函数 - 用户界面功能
void drawOnOLED(u8g2_t *u8g2);

#endif