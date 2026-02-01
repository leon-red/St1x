/**
 * @file    St1xFocusedHeating.c
 * @brief   专注加热状态机实现 - 专注加热模式状态机管理
 * @author  Leon Red
 * @date    2024-01-15
 * @version 1.0
 * 
 * @details 专注加热状态机管理实现，确保专注加热过程的专业性和稳定性
 *          兼容正常模式和校准模式下的专注加热需求
 */

#include "St1xFocusedHeating.h"
#include "St1xPID.h"
#include "tim.h"
#include <math.h>

// ==================== 全局变量定义 ====================

/** @brief 专注加热状态机实例 */
FocusedHeatingStateMachine focused_heating_sm = {
    .state = FOCUSED_HEATING_IDLE,
    .start_time = 0,
    .duration = FOCUSED_HEATING_DURATION_MS,
    .temp_diff_threshold = FOCUSED_HEATING_TEMP_DIFF_THRESHOLD,
    .is_calibration_mode = 0
};

// ==================== 状态机管理函数实现 ====================

/**
 * @brief 初始化专注加热状态机
 */
void FocusedHeating_Init(void) {
    focused_heating_sm.state = FOCUSED_HEATING_IDLE;
    focused_heating_sm.start_time = 0;
    focused_heating_sm.duration = FOCUSED_HEATING_DURATION_MS;
    focused_heating_sm.temp_diff_threshold = FOCUSED_HEATING_TEMP_DIFF_THRESHOLD;
    focused_heating_sm.is_calibration_mode = 0;
}

/**
 * @brief 启动专注加热
 * @param start_time 专注加热开始时间
 * @param is_calibration 是否在校准模式下
 */
void FocusedHeating_Start(uint32_t start_time, uint8_t is_calibration) {
    focused_heating_sm.state = FOCUSED_HEATING_ACTIVE;
    focused_heating_sm.start_time = start_time;
    focused_heating_sm.is_calibration_mode = is_calibration;
    
    // 设置PWM为最大值（100%占空比）
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
}

/**
 * @brief 停止专注加热
 */
void FocusedHeating_Stop(void) {
    focused_heating_sm.state = FOCUSED_HEATING_IDLE;
    focused_heating_sm.start_time = 0;
    
    // 注意：这里不修改PWM值，由PID控制函数负责恢复正常的PID控制
}

/**
 * @brief 更新专注加热状态机
 * @param current_time 当前时间
 * @param current_temp 当前温度
 * @param target_temp 目标温度
 * @return 是否需要专注加热（1=需要，0=不需要）
 */
uint8_t FocusedHeating_Update(uint32_t current_time, float current_temp, float target_temp) {
    // 计算温度差
    float temp_diff = target_temp - current_temp;
    
    switch (focused_heating_sm.state) {
        case FOCUSED_HEATING_IDLE:
            // 检查是否需要启动专注加热
            if (temp_diff > focused_heating_sm.temp_diff_threshold) {
                FocusedHeating_Start(current_time, focused_heating_sm.is_calibration_mode);
                return 1; // 需要专注加热
            }
            break;
            
        case FOCUSED_HEATING_ACTIVE:
            // 检查专注加热是否已完成（确保完整的2秒加热时间）
            if ((current_time - focused_heating_sm.start_time) >= focused_heating_sm.duration) {
                focused_heating_sm.state = FOCUSED_HEATING_COMPLETED;
                return 0; // 专注加热完成，不需要继续
            }
            
            // 专注加热期间不提前结束，确保完整的2秒加热时间
            // 只有在温度超过目标温度时才提前结束（安全保护）
            if (temp_diff < -5.0f) { // 温度超过目标温度5°C时提前结束（安全保护）
                focused_heating_sm.state = FOCUSED_HEATING_COMPLETED;
                return 0;
            }
            
            return 1; // 继续专注加热
            
        case FOCUSED_HEATING_COMPLETED:
        case FOCUSED_HEATING_ABORTED:
            // 检查是否需要重新启动专注加热（温度差再次大于阈值）
            if (temp_diff > focused_heating_sm.temp_diff_threshold) {
                // 重置状态到空闲，以便再次触发专注加热
                focused_heating_sm.state = FOCUSED_HEATING_IDLE;
                // 检查是否需要立即启动专注加热
                if (temp_diff > focused_heating_sm.temp_diff_threshold) {
                    FocusedHeating_Start(current_time, focused_heating_sm.is_calibration_mode);
                    return 1; // 需要专注加热
                }
            }
            // 这些状态下不需要专注加热
            return 0;
            
        default:
            break;
    }
    
    return 0;
}

/**
 * @brief 检查是否处于专注加热状态
 * @return 1=专注加热中，0=非专注加热状态
 */
uint8_t FocusedHeating_IsActive(void) {
    return (focused_heating_sm.state == FOCUSED_HEATING_ACTIVE);
}

/**
 * @brief 获取专注加热剩余时间
 * @param current_time 当前时间
 * @return 剩余时间（毫秒），0表示专注加热已完成
 */
uint32_t FocusedHeating_GetRemainingTime(uint32_t current_time) {
    if (focused_heating_sm.state != FOCUSED_HEATING_ACTIVE) {
        return 0;
    }
    
    uint32_t elapsed = current_time - focused_heating_sm.start_time;
    if (elapsed >= focused_heating_sm.duration) {
        return 0;
    }
    
    return focused_heating_sm.duration - elapsed;
}

/**
 * @brief 设置校准模式状态
 * @param is_calibration 是否在校准模式下
 */
void FocusedHeating_SetCalibrationMode(uint8_t is_calibration) {
    focused_heating_sm.is_calibration_mode = is_calibration;
    
    // 如果从校准模式切换到正常模式，且正在专注加热，则停止专注加热
    if (!is_calibration && focused_heating_sm.state == FOCUSED_HEATING_ACTIVE) {
        FocusedHeating_Stop();
    }
}

/**
 * @brief 获取专注加热状态机当前状态
 * @return 当前状态
 */
FocusedHeatingState FocusedHeating_GetState(void) {
    return focused_heating_sm.state;
}

/**
 * @brief 重置专注加热状态机到空闲状态
 */
void FocusedHeating_Reset(void) {
    focused_heating_sm.state = FOCUSED_HEATING_IDLE;
    focused_heating_sm.start_time = 0;
}

/**
 * @brief 中止专注加热（用于紧急情况）
 */
void FocusedHeating_Abort(void) {
    focused_heating_sm.state = FOCUSED_HEATING_ABORTED;
    focused_heating_sm.start_time = 0;
    
    // 设置PWM为0，立即停止加热
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
}