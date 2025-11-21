/**
 * @file    St1xFocusedHeating.h
 * @brief   专注加热状态机 - 专注加热模式状态机管理
 * @author  Leon Red
 * @date    2024-01-15
 * @version 1.0
 * 
 * @details 专注加热状态机管理，确保专注加热过程的专业性和稳定性
 *          兼容正常模式和校准模式下的专注加热需求
 */

#ifndef ST1XFOCUSEDHEATING_H
#define ST1XFOCUSEDHEATING_H

#include "stm32f1xx_hal.h"

// ==================== 专注加热状态定义 ====================

/**
 * @brief 专注加热状态枚举
 */
typedef enum {
    FOCUSED_HEATING_IDLE = 0,      /**< 空闲状态 - 未启用专注加热 */
    FOCUSED_HEATING_ACTIVE,        /**< 活跃状态 - 专注加热进行中 */
    FOCUSED_HEATING_COMPLETED,     /**< 完成状态 - 专注加热完成 */
    FOCUSED_HEATING_ABORTED        /**< 中止状态 - 专注加热被中止 */
} FocusedHeatingState;

// ==================== 专注加热配置参数 ====================

#define FOCUSED_HEATING_DURATION_MS     2000    /**< 专注加热持续时间（毫秒） */
#define FOCUSED_HEATING_TEMP_DIFF_THRESHOLD 10.0f /**< 专注加热触发温度差阈值（降低阈值，更积极启动） */
#define FOCUSED_HEATING_PWM_DUTY        10000   /**< 专注加热PWM占空比（100%） */

// ==================== 专注加热状态机结构体 ====================

/**
 * @brief 专注加热状态机结构体
 */
typedef struct {
    FocusedHeatingState state;         /**< 当前状态 */
    uint32_t start_time;               /**< 专注加热开始时间 */
    uint32_t duration;                 /**< 专注加热持续时间 */
    float temp_diff_threshold;          /**< 温度差阈值 */
    uint8_t is_calibration_mode;        /**< 是否在校准模式下 */
} FocusedHeatingStateMachine;

// ==================== 全局变量声明 ====================

extern FocusedHeatingStateMachine focused_heating_sm;

// ==================== 状态机管理函数声明 ====================

/**
 * @brief 初始化专注加热状态机
 */
void FocusedHeating_Init(void);

/**
 * @brief 启动专注加热
 * @param start_time 专注加热开始时间
 * @param is_calibration 是否在校准模式下
 */
void FocusedHeating_Start(uint32_t start_time, uint8_t is_calibration);

/**
 * @brief 停止专注加热
 */
void FocusedHeating_Stop(void);

/**
 * @brief 更新专注加热状态机
 * @param current_time 当前时间
 * @param current_temp 当前温度
 * @param target_temp 目标温度
 * @return 是否需要专注加热（1=需要，0=不需要）
 */
uint8_t FocusedHeating_Update(uint32_t current_time, float current_temp, float target_temp);

/**
 * @brief 检查是否处于专注加热状态
 * @return 1=专注加热中，0=非专注加热状态
 */
uint8_t FocusedHeating_IsActive(void);

/**
 * @brief 获取专注加热剩余时间
 * @param current_time 当前时间
 * @return 剩余时间（毫秒），0表示专注加热已完成
 */
uint32_t FocusedHeating_GetRemainingTime(uint32_t current_time);

/**
 * @brief 设置校准模式状态
 * @param is_calibration 是否在校准模式下
 */
void FocusedHeating_SetCalibrationMode(uint8_t is_calibration);

/**
 * @brief 获取专注加热状态机当前状态
 * @return 当前状态
 */
FocusedHeatingState FocusedHeating_GetState(void);

/**
 * @brief 重置专注加热状态机到空闲状态
 */
void FocusedHeating_Reset(void);

/**
 * @brief 中止专注加热（用于紧急情况）
 */
void FocusedHeating_Abort(void);

#endif // ST1XFOCUSEDHEATING_H