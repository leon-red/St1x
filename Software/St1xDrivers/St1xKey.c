#include "St1xKey.h"

// 按键状态定义
#define KEY_STATE_RELEASE 0
#define KEY_STATE_PRESS 1

// 长按时间定义（毫秒）
#define LONG_PRESS_TIME 1000  // 长按1秒进入菜单

// 按键去抖动时间
#define DEBOUNCE_TIME 50

// 按键参数导出函数（供校准系统使用）
uint32_t getKeyDebounceTimeParameter(void) {
    return DEBOUNCE_TIME;
}

uint32_t getKeyLongPressTimeParameter(void) {
    return LONG_PRESS_TIME;
}

// 菜单键长按检测变量
static uint32_t menu_key_press_time = 0;
static uint8_t menu_key_pressed = 0;
static uint8_t menu_key_handled = 0; // 标记菜单键是否已处理

// 按键状态结构体
typedef struct {
    uint8_t state;           // 按键当前状态
    uint32_t press_time;     // 按键按下时间
    uint8_t handled;         // 按键是否已处理
} KeyState;

// 按键状态变量
static KeyState key_up_state = {KEY_STATE_RELEASE, 0, 0};
static KeyState key_down_state = {KEY_STATE_RELEASE, 0, 0};
static KeyState key_mode_state = {KEY_STATE_RELEASE, 0, 0};

// 加热状态变量（从St1xADC.h中声明的变量）
extern uint8_t heating_status;
extern TIM_HandleTypeDef htim2;

// 声明静置控制模块中的手动停止标记设置函数
extern void St1xStatic_SetManuallyStopped(uint8_t stopped);

// 菜单活动状态变量（从main.c中声明的变量）
extern uint8_t menu_active;

/**
 * @brief 按键初始化函数
 */
void Key_Init(void) {
    // 初始化按键状态
    key_up_state.state = KEY_STATE_RELEASE;
    key_down_state.state = KEY_STATE_RELEASE;
    key_mode_state.state = KEY_STATE_RELEASE;
    
    key_up_state.press_time = 0;
    key_down_state.press_time = 0;
    key_mode_state.press_time = 0;
    
    key_up_state.handled = 0;
    key_down_state.handled = 0;
    key_mode_state.handled = 0;
    
    // 初始化菜单键状态
    menu_key_press_time = 0;
    menu_key_pressed = 0;
    menu_key_handled = 0;
}

/**
 * @brief 处理加热控制逻辑
 */
void handleHeatingControl(void) {
    // 先检查电压是否足够
    if (!isUSBVoltageSufficient()) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        stopHeatingControlTimer();
        heating_status = 0;
        St1xStatic_SetManuallyStopped(1);  // 手动停止加热
        return;
    }
    
    // 如果当前没有加热
    if (heating_status == 0) {
        // 开始加热
        setT12Temperature(target_temperature);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
        
        // 记录开始加热时间
        extern uint32_t heating_start_time;
        heating_start_time = HAL_GetTick();
        
        // 设置初始功率为100%
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 10000);
        
        startHeatingControlTimer();
        heating_status = 1;
        St1xStatic_SetManuallyStopped(0);  // 开始加热，重置手动停止标记
        
        // 重置采样状态机
        extern uint8_t sampling_phase;
        extern uint32_t sample_start_time;
        extern uint16_t saved_pwm_value;
        sampling_phase = 0;
        sample_start_time = 0;
        saved_pwm_value = 0;
    } else {
        // 停止加热
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
        setT12Temperature(target_temperature); // 重新设置目标温度以重置PID状态
        stopHeatingControlTimer();
        heating_status = 0;
        St1xStatic_SetManuallyStopped(1);  // 手动停止加热
        
        extern uint8_t sampling_phase;
        extern uint32_t sample_start_time;
        extern uint16_t saved_pwm_value;
        sampling_phase = 0;
        sample_start_time = 0;
        saved_pwm_value = 0;
    }
}

/**
 * @brief 处理温度调节逻辑
 * @param direction 调节方向，1为增加，-1为减少
 */
void handleTemperatureAdjust(int direction) {
    if (direction > 0) {
        // 增加5度
        target_temperature += 5.0;

        // 温度上限保护
        extern float max_temperature_limit;
        if (target_temperature > max_temperature_limit) {
            target_temperature = max_temperature_limit;
        }
    } else {
        // 减少5度
        target_temperature -= 5.0;

        // 温度下限保护
        if (target_temperature < 0.0) {
            target_temperature = 0.0;
        }
    }

    // 如果正在加热，更新目标温度
    if (heating_status == 1) {
        // 更新PID控制器的目标温度
        setT12Temperature(target_temperature);
    }
}

/**
 * @brief 处理主界面的温度调节按键
 * @param key 按键类型
 */
void handleMainTemperatureAdjust(KeyType key) {
    switch (key) {
        case KEY_UP:
            // 增加目标温度
            target_temperature += 5.0;
            extern float max_temperature_limit;
            if (target_temperature > max_temperature_limit) {
                target_temperature = max_temperature_limit;
            }
            // 更新PID控制器的目标温度
            setT12Temperature(target_temperature);
            break;

        case KEY_DOWN:
            // 减少目标温度
            target_temperature -= 5.0;
            if (target_temperature < 0.0) {
                target_temperature = 0.0;
            }
            // 更新PID控制器的目标温度
            setT12Temperature(target_temperature);
            break;

        default:
            break;
    }
}



/**
 * @brief 按键扫描函数（改进版 - 支持防抖和长按）
 * @return 按键类型
 */
KeyType Key_Scan(void) {
    uint32_t current_time = HAL_GetTick();
    KeyType key_pressed = KEY_NONE;
    
    // 扫描向上按键 KEY_UP_Pin
    if (HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_RESET) {
        if (key_up_state.state == KEY_STATE_RELEASE) {
            // 按键刚按下，记录时间
            key_up_state.state = KEY_STATE_PRESS;
            key_up_state.press_time = current_time;
            key_up_state.handled = 0;
        } else if (key_up_state.state == KEY_STATE_PRESS && !key_up_state.handled) {
            // 按键持续按下，检查是否达到防抖时间
            if ((current_time - key_up_state.press_time) >= DEBOUNCE_TIME) {
                key_pressed = KEY_UP;
                key_up_state.handled = 1;  // 标记为已处理
            }
        }
    } else {
        if (key_up_state.state == KEY_STATE_PRESS) {
            // 按键释放，重置状态
            key_up_state.state = KEY_STATE_RELEASE;
            key_up_state.handled = 0;
        }
    }
    
    // 扫描向下按键 KEY_DOWN_Pin
    if (HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port, KEY_DOWN_Pin) == GPIO_PIN_RESET) {
        if (key_down_state.state == KEY_STATE_RELEASE) {
            // 按键刚按下，记录时间
            key_down_state.state = KEY_STATE_PRESS;
            key_down_state.press_time = current_time;
            key_down_state.handled = 0;
        } else if (key_down_state.state == KEY_STATE_PRESS && !key_down_state.handled) {
            // 按键持续按下，检查是否达到防抖时间
            if ((current_time - key_down_state.press_time) >= DEBOUNCE_TIME) {
                key_pressed = KEY_DOWN;
                key_down_state.handled = 1;  // 标记为已处理
            }
        }
    } else {
        if (key_down_state.state == KEY_STATE_PRESS) {
            // 按键释放，重置状态
            key_down_state.state = KEY_STATE_RELEASE;
            key_down_state.handled = 0;
        }
    }
    
    // 扫描模式按键 KEY_MODE_Pin（特殊处理：支持短按和长按）
    if (HAL_GPIO_ReadPin(KEY_MODE_GPIO_Port, KEY_MODE_Pin) == GPIO_PIN_RESET) {
        if (key_mode_state.state == KEY_STATE_RELEASE) {
            // 按键刚按下，记录时间
            key_mode_state.state = KEY_STATE_PRESS;
            key_mode_state.press_time = current_time;
            key_mode_state.handled = 0;
        } else if (key_mode_state.state == KEY_STATE_PRESS && !key_mode_state.handled) {
            // 检查是否达到长按时间
            if ((current_time - key_mode_state.press_time) >= LONG_PRESS_TIME) {
                key_pressed = KEY_MODE_LONG;
                key_mode_state.handled = 1;  // 标记为已处理
            }
            // 注意：短按检测在按键释放时处理，避免与长按冲突
        }
    } else {
        if (key_mode_state.state == KEY_STATE_PRESS) {
            // 按键释放，检查是否达到短按时间
            // 只有在没有检测到长按的情况下才返回短按
            if (!key_mode_state.handled && (current_time - key_mode_state.press_time) >= DEBOUNCE_TIME && 
                (current_time - key_mode_state.press_time) < LONG_PRESS_TIME) {
                key_pressed = KEY_MODE;
                key_mode_state.handled = 1;  // 标记为已处理
            }
            // 重置状态
            key_mode_state.state = KEY_STATE_RELEASE;
            key_mode_state.handled = 0;
        }
    }
    
    return key_pressed;

}

/**
 * @brief 获取按键编号（兼容旧接口）
 * @return 按键编号
 */
uint8_t Key_GetNum_New(void) {
    KeyType key = Key_Scan();
    
    switch (key) {
        case KEY_UP:
            return 1;
        case KEY_DOWN:
            return 2;
        case KEY_MODE:
            return 3;
        case KEY_MODE_LONG:
            return 4;
        default:
            return 0;
    }
}