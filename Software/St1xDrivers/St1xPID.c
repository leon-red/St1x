/**
 * @file    St1xPID.c
 * @brief   T12烙铁控制器 - 温度控制实现
 * @author  Leon Red
 * @date    2023-02-13
 */

#include "St1xPID.h"
#include "St1xADC.h"
#include "St1xFocusedHeating.h"
#include "adc.h"
#include "tim.h"
#include <math.h>
#include <stdio.h>
#include "St1xCalibrationSystem.h"

// 外部变量声明
extern uint16_t DMA_ADC[3];
extern float target_temperature;
extern uint32_t last_control_time;
extern uint32_t heating_start_time;
extern uint8_t heating_control_enabled;
extern float display_filtered_temperature;
extern uint8_t heating_status;
extern uint8_t sampling_phase;
extern uint32_t sample_start_time;
extern uint16_t saved_pwm_value;
extern uint32_t initial_heating_end_time;

// PID控制参数
#define PID_CONSERVATIVE_KP 8.0f    // 保守模式比例系数
#define PID_CONSERVATIVE_KI 2.5f    // 保守模式积分系数
#define PID_CONSERVATIVE_KD 12.0f   // 保守模式微分系数
#define CONTROL_INTERVAL 50         // 控制间隔（毫秒）- 每50ms执行一次完整的控制周期
#define FOCUSED_HEATING_TEMP_DIFF 15.0f // 专注加热温度差阈值 - 当目标温度与当前温度差大于15°C时启用专注加热
#define AGGRESSIVE_HEATING_TEMP_DIFF 5.0f // 激进加热温度差阈值 - 温度差大于5°C时使用100%功率加热
#define PID_PRECISION_THRESHOLD 2.0f  // PID精确控制阈值 - 温度差小于2°C时启用精确PID参数

// 冷启动专用参数
#define COLD_START_AGGRESSIVE_KP 15.0f    // 冷启动激进比例系数
#define COLD_START_AGGRESSIVE_KI 0.8f     // 冷启动激进积分系数
#define COLD_START_AGGRESSIVE_KD 8.0f     // 冷启动激进微分系数

// 全局变量
PID_Controller t12_pid = {0.5, 0.01, 1.2, 0.0, 0.0, 0.0, 0};

// 专注加热模式标志（保持兼容性，但实际使用状态机）
uint8_t focused_heating_mode = 0;

// PID温度控制算法
float pidTemperatureControl(float current_temp) {
    uint32_t current_time = HAL_GetTick();
    float dt = (current_time - t12_pid.last_time) / 1000.0f;
    if (dt <= 0) dt = 0.01f;
    
    float error = t12_pid.setpoint - current_temp;
    
    // 冷启动模式检查 - 使用更激进的加热策略
    extern uint8_t isColdStartMode(void);
    if (isColdStartMode() && error > 20.0f) {
        // 冷启动模式下，温度差大于20°C时使用100%功率加热
        t12_pid.last_time = current_time;
        return 100.0f;
    }
    
    // 专注加热模式检查 - 使用状态机管理
    if (heating_status && heating_control_enabled && error > FOCUSED_HEATING_TEMP_DIFF) {
        // 使用状态机检查是否需要专注加热
        if (FocusedHeating_Update(current_time, current_temp, t12_pid.setpoint)) {
            focused_heating_mode = 1; // 保持兼容性
            t12_pid.last_time = current_time;
            return 100.0f;
        } else {
            focused_heating_mode = 0; // 专注加热完成或不需要
        }
    }
    
    // 激进加热模式
    if (error > AGGRESSIVE_HEATING_TEMP_DIFF && !focused_heating_mode) {
        t12_pid.last_time = current_time;
        return 100.0f;
    }
    
    if (error < -AGGRESSIVE_HEATING_TEMP_DIFF) {
        t12_pid.last_time = current_time;
        return 0.0f;
    }
    
    // 精确PID模式 - 根据冷启动状态选择参数
    if (fabs(error) < PID_PRECISION_THRESHOLD) {
        if (isColdStartMode()) {
            t12_pid.kp = COLD_START_AGGRESSIVE_KP * 0.6f;
            t12_pid.ki = COLD_START_AGGRESSIVE_KI * 1.5f;
            t12_pid.kd = COLD_START_AGGRESSIVE_KD * 1.2f;
        } else {
            t12_pid.kp = PID_CONSERVATIVE_KP * 0.6f;
            t12_pid.ki = PID_CONSERVATIVE_KI * 1.5f;
            t12_pid.kd = PID_CONSERVATIVE_KD * 1.2f;
        }
    } else {
        if (isColdStartMode()) {
            t12_pid.kp = COLD_START_AGGRESSIVE_KP;
            t12_pid.ki = COLD_START_AGGRESSIVE_KI;
            t12_pid.kd = COLD_START_AGGRESSIVE_KD;
        } else {
            t12_pid.kp = PID_CONSERVATIVE_KP;
            t12_pid.ki = PID_CONSERVATIVE_KI;
            t12_pid.kd = PID_CONSERVATIVE_KD;
        }
    }
    
    // PID计算
    float proportional = t12_pid.kp * error;
    t12_pid.integral += error * dt;
    
    // 智能积分衰减
    float integral_decay_factor = (fabs(error) < 5.0f) ? 0.95f : (fabs(error) < 10.0f) ? 0.9f : 0.8f;
    t12_pid.integral *= integral_decay_factor;
    
    // 积分限幅
    if (t12_pid.integral > 20) t12_pid.integral = 20;
    if (t12_pid.integral < -20) t12_pid.integral = -20;
    
    float integral = t12_pid.ki * t12_pid.integral;
    float derivative = t12_pid.kd * (error - t12_pid.prev_error) / dt;
    float output = proportional + integral + derivative;
    
    // 输出限幅
    if (output > 100.0) output = 100.0;
    if (output < 0.0) output = 0.0;
    
    t12_pid.prev_error = error;
    t12_pid.last_time = current_time;
    
    return output;
}

// 设置目标温度
void setT12Temperature(float temperature) {
    t12_pid.setpoint = temperature;
    t12_pid.integral = 0.0;
    t12_pid.prev_error = 0.0;
    t12_pid.last_time = HAL_GetTick();
}

// 获取PID参数
float getPID_Kp(void) { return t12_pid.kp; }
float getPID_Ki(void) { return t12_pid.ki; }
float getPID_Kd(void) { return t12_pid.kd; }

// 启动/停止温度控制定时器
void startHeatingControlTimer(void) {
    heating_control_enabled = 1;
    HAL_TIM_Base_Start_IT(&htim2);
}

void stopHeatingControlTimer(void) {
    heating_control_enabled = 0;
    HAL_TIM_Base_Stop_IT(&htim2);
}

// 定时器中断回调函数
// 功能：加热控制定时器中断处理函数，实现温度采样和PID控制
// 调用时机：定时器中断触发时自动调用
void heatingControlTimerCallback(void) {
    uint32_t current_time = HAL_GetTick();
    
    // 安全检查：如果加热状态为0或控制未启用，直接返回
    if (heating_status == 0 || heating_control_enabled == 0) {
        return;
    }

    // 冷启动模式检查 - 在每次控制循环中更新冷启动状态
    extern void checkAndEnterColdStartMode(float current_temp);
    extern void updateColdJunctionTemperature(void);
    extern float filtered_temperature;
    
    // 更新冷端补偿温度
    updateColdJunctionTemperature();
    
    // 检查并更新冷启动模式状态
    checkAndEnterColdStartMode(filtered_temperature);

    // 专注加热模式处理 - 使用状态机管理
    // 专注加热模式用于快速升温，当温度差较大时启用
    if (FocusedHeating_IsActive()) {
        // 专注加热期间，设置PWM为最大值（100%占空比）
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
    }

    // 控制间隔检查：确保控制周期为50ms
    if ((current_time - last_control_time) < CONTROL_INTERVAL) {
        return;
    }
    
    // 更新最后控制时间
    last_control_time = current_time;
    
    // 采样阶段状态机
    // 优化采样策略：专注加热期间减少采样中断
    static uint32_t last_sample_time = 0;
    static uint8_t sample_counter = 0;
    
    switch (sampling_phase) {
        case 0: // 准备采样阶段
            // 检查是否处于专注加热模式
            if (FocusedHeating_IsActive()) {
                // 专注加热期间：降低采样频率，每4个控制周期采样一次（12.5Hz）
                sample_counter++;
                if (sample_counter >= 4) {
                    sample_counter = 0;
                    // 需要采样：采用简化采样流程，不关闭加热
                    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 3);
                    // 更新温度滤波器
                    updateTemperatureFilter(DMA_ADC[0]);
                    last_sample_time = current_time;
                } else {
                    // 不需要采样：直接跳过采样阶段，保持最大功率
                    // 专注加热期间保持最大功率
                    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
                    // 直接进入控制计算阶段
                    sampling_phase = 3;
                    break;
                }
                // 专注加热期间保持最大功率
                __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
                // 直接进入控制计算阶段
                sampling_phase = 3;
            } else {
                // 正常模式：采用四阶段采样策略，避免加热干扰温度测量
                // 保存当前PWM值，用于采样后恢复
                saved_pwm_value = __HAL_TIM_GET_COMPARE(&htim2, TIM_CHANNEL_2);
                // 关闭加热，准备采样（PWM设为0）
                __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
                // 记录采样开始时间
                sample_start_time = current_time;
                // 进入等待稳定阶段
                sampling_phase = 1;
            }
            break;
            
        case 1: // 等待稳定阶段
            // 等待1ms让温度传感器稳定
            if ((current_time - sample_start_time) >= 1) {
                sampling_phase = 2; // 进入执行采样阶段
            }
            break;
            
        case 2: // 执行采样阶段
            // 启动ADC DMA采样，采集3个通道的数据
            HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 3);
            // 更新温度滤波器，使用第一个通道的数据（温度传感器）
            updateTemperatureFilter(DMA_ADC[0]);
            // 进入恢复加热阶段
            sampling_phase = 3;
            break;
            
        case 3: // 恢复加热阶段
            {
                // 获取滤波后的温度值
                extern float filtered_temperature;
                float control_temp = filtered_temperature;
                
                // 根据当前状态选择控制策略
                if (heating_status && heating_control_enabled) {
                    // 检查是否需要专注加热
                    float error = t12_pid.setpoint - control_temp;
                    
                    // 使用状态机管理专注加热
                    if (FocusedHeating_Update(current_time, control_temp, t12_pid.setpoint)) {
                        // 专注加热模式，保持最大功率
                        focused_heating_mode = 1; // 保持兼容性
                        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
                    } else {
                        // 正常PID控制模式
                        focused_heating_mode = 0; // 保持兼容性
                        float pwm_duty = pidTemperatureControl(control_temp);
                        // 将占空比转换为PWM计数值（10000对应100%）
                        uint16_t pwm_value = (uint16_t)(pwm_duty * 10000 / 100);
                        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, pwm_value);
                    }
                } else {
                    // 其他情况使用PID控制
                    float pwm_duty = pidTemperatureControl(control_temp);
                    uint16_t pwm_value = (uint16_t)(pwm_duty * 10000 / 100);
                    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, pwm_value);
                }
                // 完成一个完整的控制周期，回到准备采样阶段
                sampling_phase = 0;
            }
            break;
    }
}