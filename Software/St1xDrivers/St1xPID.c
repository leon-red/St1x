/**
 * @file    St1xPID.c
 * @brief   T12烙铁控制器 - 温度控制实现文件
 * @author  Leon Red
 * @date    2023-02-13
 * @version 1.0
 * 
 * @details 这个文件实现了T12烙铁的温度控制功能
 *          主要功能包括：
 *          1. PID控制算法（根据当前温度和目标温度计算加热功率）
 *          2. 温度控制定时器（定期执行温度控制逻辑）
 *          3. 加热控制功能（启动/停止加热）
 */

// ==================== 包含必要的工具箱 ====================
#include "St1xPID.h"   // 我们自己的PID控制功能
#include "St1xADC.h"   // 温度测量功能
#include "adc.h"       // 传感器读取功能
#include "tim.h"       // 定时器功能
#include <math.h>      // 数学计算功能

// ==================== 声明外部变量 ====================
// 这些变量在其他文件中定义，但我们需要在这里使用它们
extern uint16_t DMA_ADC[2];               // 传感器数据
extern float target_temperature;          // 目标温度
extern uint32_t last_control_time;        // 上次控制时间
extern uint32_t heating_start_time;       // 开始加热时间
extern uint8_t heating_control_enabled;   // 控制使能标志
extern float display_filtered_temperature; // 显示用温度
extern uint8_t heating_status;            // 加热状态

// ==================== 定义常量参数 ====================
// PID控制参数（参考Arduino示例，优化响应特性）
#define PID_CONSERVATIVE_KP 15.0f // 保守模式比例系数
#define PID_CONSERVATIVE_KI 5.0f  // 保守模式积分系数
#define PID_CONSERVATIVE_KD 8.0f  // 保守模式微分系数

// 控制阈值
#define CONTROL_INTERVAL 50           // 控制间隔（毫秒）（保持与ADC文件一致）
#define FOCUSED_HEATING_DURATION 1500 // 专注加热持续时间（毫秒）（1.5秒全功率加热）
#define FOCUSED_HEATING_TEMP_DIFF 20.0f // 专注加热温度差阈值（温度低于目标值20度时启用）
#define AGGRESSIVE_HEATING_TEMP_DIFF 3.0f // 激进加热温度差阈值（温度低于目标值3度时启用）

// ==================== 定义全局变量 ====================
// PID控制器实例，初始化参数：kp=0.5, ki=0.01, kd=1.2
PID_Controller t12_pid = {0.5, 0.01, 1.2, 0.0, 0.0, 0.0, 0};

// 添加专注加热状态变量
uint8_t focused_heating_mode = 0;  // 专注加热模式标志
static uint32_t focused_heating_start_time = 0;  // 专注加热开始时间

// ==================== 声明外部变量 ====================
// 这些变量在ADC文件中定义，但我们在这里使用
extern uint8_t sampling_phase;    // 采样阶段
extern uint32_t sample_start_time; // 阶段开始时间
extern uint16_t saved_pwm_value;   // 保存的PWM值
extern uint32_t initial_heating_end_time; // 初始加热结束时间

/**
 * pidTemperatureControl - PID温度控制算法
 * 
 * 功能：根据当前温度和目标温度计算需要的加热功率
 * 原理：使用PID控制算法自动调整加热功率
 * 策略：
 *  1. 专注加热模式：温度低于目标值30度时启用，每次持续3秒全功率加热
 *  2. 激进模式：温度低于目标值20度时启用全功率加热
 *  3. 保守模式：温度接近目标值时使用PID精确控制
 * 
 * @param current_temp 当前温度
 * @return 加热功率（0.0-100.0%）
 */
float pidTemperatureControl(float current_temp) {
    uint32_t current_time = HAL_GetTick();
    float dt = (current_time - t12_pid.last_time) / 1000.0f;
    
    // 防止时间间隔为0
    if (dt <= 0) dt = 0.01f;
    
    // 计算温度误差
    float error = t12_pid.setpoint - current_temp;
    
    // 检查是否应该启用专注加热模式（温度低于目标值）
    if (error > FOCUSED_HEATING_TEMP_DIFF && !focused_heating_mode) {
        // 启动专注加热模式
        focused_heating_mode = 1;
        focused_heating_start_time = current_time;
    }
    
    // 如果在专注加热模式下
    if (focused_heating_mode) {
        // 注意：专注加热的实际控制在heatingControlTimerCallback中处理
        // 这里只是保持状态更新
        t12_pid.last_time = current_time;
        return 100.0f;  // 返回值实际上不会被使用，因为专注加热在回调函数中处理
    }
    
    // 策略2：激进模式（温度低于目标值）
    if (error > AGGRESSIVE_HEATING_TEMP_DIFF) {
        t12_pid.last_time = current_time;
        return 100.0f;  // 全功率加热
    }
    
    // 策略3：保守模式（温度接近目标值时使用PID精确控制）
    // 重新设置为保守PID参数
    t12_pid.kp = PID_CONSERVATIVE_KP;
    t12_pid.ki = PID_CONSERVATIVE_KI;
    t12_pid.kd = PID_CONSERVATIVE_KD;
    
    // 标准PID计算
    float proportional = t12_pid.kp * error;
    
    // 积分项计算（带衰减）
    t12_pid.integral += error * dt;
    float integral_decay_factor = 1.0f;
    if (fabs(error) < 10.0f) {
        integral_decay_factor = fabs(error) / 10.0f;
        if (integral_decay_factor < 0.2f) integral_decay_factor = 0.2f;
    }
    t12_pid.integral *= integral_decay_factor;
    
    // 积分限幅
    if (t12_pid.integral > 30) t12_pid.integral = 30;
    if (t12_pid.integral < -30) t12_pid.integral = -30;
    float integral = t12_pid.ki * t12_pid.integral;
    
    // 微分项计算
    float derivative = t12_pid.kd * (error - t12_pid.prev_error) / dt;
    
    // 计算总输出
    float output = proportional + integral + derivative;
    
    // 输出限幅
    if (output > 100.0) output = 100.0;
    if (output < 0.0) output = 0.0;
    
    // 保存当前误差和时间
    t12_pid.prev_error = error;
    t12_pid.last_time = current_time;
    
    return output;
}

/**
 * setT12Temperature - 设置目标温度
 * 
 * 功能：设置想要达到的温度，并重置PID控制器状态
 * 原理：改变目标温度后需要重置PID的积分和误差历史
 * 
 * @param temperature 目标温度
 */
void setT12Temperature(float temperature) {
    t12_pid.setpoint = temperature;       // 设置新目标温度
    t12_pid.integral = 0.0;               // 清空积分项
    t12_pid.prev_error = 0.0;             // 清空上次误差
    t12_pid.last_time = HAL_GetTick();    // 更新时间戳
}

/**
 * startHeatingControlTimer - 启动温度控制定时器
 * 
 * 功能：启动自动温度控制
 * 原理：开启定时器中断，定期执行温度控制逻辑
 */
void startHeatingControlTimer(void) {
    heating_control_enabled = 1;
    HAL_TIM_Base_Start_IT(&htim2);  // 启动定时器中断
}

/**
 * stopHeatingControlTimer - 停止温度控制定时器
 * 
 * 功能：停止自动温度控制
 * 原理：关闭定时器中断，停止温度控制逻辑
 */
void stopHeatingControlTimer(void) {
    heating_control_enabled = 0;
    HAL_TIM_Base_Stop_IT(&htim2);  // 停止定时器中断
}

/**
 * heatingControlTimerCallback - 定时器中断回调函数
 * 
 * 功能：定时执行温度控制逻辑
 * 原理：按固定时间间隔读取温度并调整加热功率
 * 过程：
 *  1. 准备采样：关闭加热，保存当前状态
 *  2. 等待稳定：等待信号稳定
 *  3. 执行采样：读取传感器数据
 *  4. 恢复加热：根据计算结果调整加热功率
 */
void heatingControlTimerCallback(void) {
    uint32_t current_time = HAL_GetTick();
    
    // 只有在加热状态下才执行控制逻辑
    if (heating_status == 0) {
        return;
    }

    // 检查是否在专注加热模式下
    if (focused_heating_mode) {
        // 检查专注加热是否完成
        if ((current_time - focused_heating_start_time) >= FOCUSED_HEATING_DURATION) {
            // 专注加热完成，退出专注加热模式
            focused_heating_mode = 0;
        } else {
            // 仍在专注加热中，设置全功率并返回
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
            return;
        }
    }

    // 检查是否到了控制时间
    if ((current_time - last_control_time) < CONTROL_INTERVAL) {
        return;
    }
    
    last_control_time = current_time;
    
    // 分阶段执行采样和控制
    switch (sampling_phase) {
        case 0: // 准备采样阶段
            saved_pwm_value = __HAL_TIM_GET_COMPARE(&htim2, TIM_CHANNEL_2);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);  // 关闭加热进行采样
            sample_start_time = current_time;
            sampling_phase = 1;
            break;
            
        case 1: // 等待信号稳定
            // 缩短等待时间以提高响应速度
            if ((current_time - sample_start_time) >= 1) {
                sampling_phase = 2;
            }
            break;
            
        case 2: // 执行采样
            HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&DMA_ADC, 2);
            updateTemperatureFilter(DMA_ADC[0]);
            sampling_phase = 3;
            break;
            
        case 3: // 恢复加热
            {
                float control_temp = getFilteredTemperature();
                float pwm_duty = pidTemperatureControl(control_temp);
                
                // 检查是否应该启动专注加热模式
                float error = t12_pid.setpoint - control_temp;
                if (error > FOCUSED_HEATING_TEMP_DIFF && !focused_heating_mode) {
                    // 启动专注加热模式
                    focused_heating_mode = 1;
                    focused_heating_start_time = current_time;
                    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000); // 立即设置为全功率
                } else {
                    // 设置新的加热功率
                    uint16_t pwm_value = (uint16_t)(pwm_duty * 10000 / 100);
                    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, pwm_value);
                }
                sampling_phase = 0;  // 重置状态机
            }
            break;
    }
}