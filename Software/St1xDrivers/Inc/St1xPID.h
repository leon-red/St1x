/**
 * @file    St1xPID.h
 * @brief   T12烙铁控制器 - 温度控制头文件
 * @author  Leon Red
 * @date    2023-02-13
 * @version 1.0
 * 
 * @details 这个文件是用来控制温度的部分，就像一个智能调温器
 *          主要功能：
 *          1. 根据当前温度和想要的温度来决定加热功率
 *          2. 定时检查和调整温度
 *          3. 控制什么时候开始加热，什么时候停止加热
 */

#ifndef ST1XPID_H
#define ST1XPID_H

#include "stm32f1xx_hal.h"
#include "St1xADC.h"

// ==================== 数据结构定义 ====================

/**
 * @brief 温度控制器
 * @details 这是一个智能的温度控制算法，会根据温度差自动调整加热功率
 *          就像你手动调温度，但这个是自动的
 */
typedef struct {
    float kp;           /**< 比例项 - 温度差越大加热越猛 */
    float ki;           /**< 积分项 - 消除长时间的小误差 */
    float kd;           /**< 微分项 - 防止温度超过目标值太多 */
    float setpoint;     /**< 目标温度 - 你想让烙铁达到多少度 */
    float integral;     /**< 积分值 - 记住之前的误差 */
    float prev_error;   /**< 上次误差 - 用来计算变化速度 */
    uint32_t last_time; /**< 上次时间 - 记录控制时间 */
} PID_Controller;

// ==================== 全局变量声明 ====================

/** @brief 温度控制器实例 */
extern PID_Controller t12_pid;

// 声明专注加热模式标志，供ADC模块使用
extern uint8_t focused_heating_mode;

// ==================== 核心功能函数声明 ====================

/**
 * @brief 计算加热功率
 * @param current_temp 当前温度
 * @return 加热功率（0-100%）
 * @details 看看现在多少度，想想应该用多大功率加热
 */
float pidTemperatureControl(float current_temp);

/**
 * @brief 设置想要的温度
 * @param temperature 目标温度
 * @details 告诉控制器你想把烙铁调到多少度
 */
void setT12Temperature(float temperature);

// ==================== 定时器控制函数声明 ====================

/**
 * @brief 开始自动控温
 * @details 启动定时器，开始自动控制温度
 */
void startHeatingControlTimer(void);

/**
 * @brief 停止自动控温
 * @details 停止定时器，不再自动控制温度
 */
void stopHeatingControlTimer(void);

/**
 * @brief 定时器回调函数
 * @details 定时器到时间就执行这个函数，用来控制温度
 */
void heatingControlTimerCallback(void);

// ==================== PID参数获取函数声明 ====================

/**
 * @brief 获取当前PID比例系数
 * @return 当前比例系数值
 */
float getPID_Kp(void);

/**
 * @brief 获取当前PID积分系数
 * @return 当前积分系数值
 */
float getPID_Ki(void);

/**
 * @brief 获取当前PID微分系数
 * @return 当前微分系数值
 */
float getPID_Kd(void);

#endif