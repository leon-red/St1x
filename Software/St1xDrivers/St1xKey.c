#include "St1xKey.h"
#include "u8g2.h"  // 添加u8g2头文件包含

// 按键状态定义
#define KEY_STATE_RELEASE 0
#define KEY_STATE_PRESS 1

// 长按时间定义（毫秒）
#define LONG_PRESS_TIME 1000

// 按键去抖动时间
#define DEBOUNCE_TIME 50

// 菜单键长按检测变量
#define MENU_KEY_LONG_PRESS_TIME 2000  // 长按2秒进入菜单
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
        if (target_temperature > 460.0) {
            target_temperature = 460.0;
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
        // 这里可以添加更新温度控制逻辑的代码
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
            if (target_temperature > 460.0) {
                target_temperature = 460.0;
            }
            break;
            
        case KEY_DOWN:
            // 减少目标温度
            target_temperature -= 5.0;
            if (target_temperature < 0.0) {
                target_temperature = 0.0;
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 菜单键处理函数
 */
void handleMenuKey(void) {
    uint32_t current_time = HAL_GetTick();
    
    // 检查菜单键状态
    if (HAL_GPIO_ReadPin(KEY_MODE_GPIO_Port, KEY_MODE_Pin) == GPIO_PIN_RESET) {
        // 按键按下
        if (!menu_key_pressed) {
            // 首次按下，记录时间
            menu_key_press_time = current_time;
            menu_key_pressed = 1;
            menu_key_handled = 0; // 重置处理标志
        } else if (!menu_key_handled) {
            // 持续按下，检查是否超过长按时间
            if ((current_time - menu_key_press_time) >= MENU_KEY_LONG_PRESS_TIME) {
                // 长按2秒，立即进入菜单，无需等待按键释放
                Menu_InitSystem();
                menu_active = 1;
                menu_key_handled = 1; // 标记已处理
            }
        }
    } else {
        // 按键释放
        menu_key_pressed = 0;
    }
}

/**
 * @brief 按键扫描函数
 * @return 按键类型
 */
KeyType Key_Scan(void) {
    uint32_t current_time = HAL_GetTick();
    KeyType key_pressed = KEY_NONE;
    
    // 扫描向上按键 KEY_UP_Pin
    if (HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_RESET) {
        if (key_up_state.state == KEY_STATE_RELEASE) {
            // 按键刚按下
            HAL_Delay(DEBOUNCE_TIME); // 简单去抖动
            if (HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_RESET) {
                key_up_state.state = KEY_STATE_PRESS;
                key_up_state.press_time = current_time;
                key_up_state.handled = 0;
            }
        }
    } else {
        if (key_up_state.state == KEY_STATE_PRESS) {
            // 按键刚释放
            key_up_state.state = KEY_STATE_RELEASE;
            if (!key_up_state.handled) {
                key_pressed = KEY_UP;
                key_up_state.handled = 1;
            }
        }
    }
    
    // 扫描向下按键 KEY_DOWN_Pin
    if (HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port, KEY_DOWN_Pin) == GPIO_PIN_RESET) {
        if (key_down_state.state == KEY_STATE_RELEASE) {
            // 按键刚按下
            HAL_Delay(DEBOUNCE_TIME); // 简单去抖动
            if (HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port, KEY_DOWN_Pin) == GPIO_PIN_RESET) {
                key_down_state.state = KEY_STATE_PRESS;
                key_down_state.press_time = current_time;
                key_down_state.handled = 0;
            }
        }
    } else {
        if (key_down_state.state == KEY_STATE_PRESS) {
            // 按键刚释放
            key_down_state.state = KEY_STATE_RELEASE;
            if (!key_down_state.handled) {
                key_pressed = KEY_DOWN;
                key_down_state.handled = 1;
            }
        }
    }
    
    // 扫描模式按键 KEY_MODE_Pin
    if (HAL_GPIO_ReadPin(KEY_MODE_GPIO_Port, KEY_MODE_Pin) == GPIO_PIN_RESET) {
        if (key_mode_state.state == KEY_STATE_RELEASE) {
            // 按键刚按下
            key_mode_state.state = KEY_STATE_PRESS;
            key_mode_state.press_time = current_time;
            key_mode_state.handled = 0;
        } else if (!key_mode_state.handled) {
            // 按键持续按下，检查是否长按
            if ((current_time - key_mode_state.press_time) >= LONG_PRESS_TIME) {
                key_pressed = KEY_MODE_LONG;
                key_mode_state.handled = 1;
            }
        }
    } else {
        if (key_mode_state.state == KEY_STATE_PRESS) {
            // 按键刚释放
            key_mode_state.state = KEY_STATE_RELEASE;
            if (!key_mode_state.handled) {
                // 短按，处理加热控制
                handleHeatingControl();
                key_pressed = KEY_MODE;
                key_mode_state.handled = 1;
            }
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