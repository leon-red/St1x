// 蜂鸣器控制模块 - 非阻塞式优化版本
// 提供5种预设提示音和自定义频率功能，不影响PID和ADC

#include "Buzzer.h"
#include "main.h"
#include "tim.h"

// 蜂鸣器状态机变量
static struct {
    uint8_t active;           // 蜂鸣器是否激活
    uint16_t frequency;       // 当前频率
    uint32_t duration_ms;     // 总持续时间
    uint32_t start_time;      // 开始时间
    uint16_t half_period;     // 半周期（微秒）
    uint32_t cycles;          // 总周期数
    uint32_t current_cycle;   // 当前周期
    uint8_t pin_state;        // 引脚状态
    uint32_t last_toggle;     // 上次切换时间
} buzzer_state = {0};

// 非阻塞式蜂鸣器初始化
void buzzerInit(void)
{
    buzzer_state.active = 0;
    buzzer_state.pin_state = 0;
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
}

// 非阻塞式蜂鸣器处理函数（需要在主循环中定期调用）
void buzzerProcess(void)
{
    if (!buzzer_state.active) return;
    
    uint32_t current_time = HAL_GetTick();
    
    // 检查是否超时
    if ((current_time - buzzer_state.start_time) >= buzzer_state.duration_ms) {
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
        buzzer_state.active = 0;
        return;
    }
    
    // 非阻塞式方波生成
    uint32_t elapsed_us = (current_time - buzzer_state.start_time) * 1000;
    uint32_t cycle_time = elapsed_us % (buzzer_state.half_period * 2);
    
    if (cycle_time < buzzer_state.half_period) {
        if (!buzzer_state.pin_state) {
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
            buzzer_state.pin_state = 1;
        }
    } else {
        if (buzzer_state.pin_state) {
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
            buzzer_state.pin_state = 0;
        }
    }
}

// 蜂鸣器发声函数（非阻塞式）
void buzzerBeep(uint16_t frequency, uint16_t duration_ms)
{
    if (buzzer_state.active) return; // 如果正在发声，忽略新请求
    
    buzzer_state.frequency = frequency;
    buzzer_state.duration_ms = duration_ms;
    buzzer_state.half_period = 500000 / frequency; // 计算半周期（微秒）
    buzzer_state.start_time = HAL_GetTick();
    buzzer_state.active = 1;
    buzzer_state.pin_state = 0;
    
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0); // 确保初始状态为低
}

// 1. 短促提示音（频率4000Hz，持续300ms）
void buzzerShortBeep(void)
{
    buzzerBeep(4000, 300);
}

// 2. 确认音（频率3500Hz，持续500ms）
void buzzerConfirmBeep(void)
{
    buzzerBeep(3500, 500);
}

// 3. 错误提示音（频率3000Hz，持续800ms）
void buzzerErrorBeep(void)
{
    buzzerBeep(3000, 800);
}

// 4. 警告音（频率5000Hz，持续400ms，重复2次）
void buzzerWarningBeep(void)
{
    static uint8_t warning_count = 0;
    static uint32_t warning_start_time = 0;
    
    if (warning_count == 0) {
        // 开始第一次警告音
        buzzerBeep(5000, 400);
        warning_count = 1;
        warning_start_time = HAL_GetTick();
    } else if (warning_count == 1) {
        // 检查是否需要开始第二次警告音
        if ((HAL_GetTick() - warning_start_time) >= 500) { // 400ms发声 + 100ms间隔
            buzzerBeep(5000, 400);
            warning_count = 2;
        }
    } else {
        // 警告音序列完成
        warning_count = 0;
    }
}

// 5. 启动音（频率从3000Hz到6000Hz，持续1200ms）
void buzzerStartupBeep(void)
{
    static uint16_t startup_freq = 3000;
    static uint32_t startup_start_time = 0;
    
    if (startup_freq == 3000) {
        // 开始启动音
        buzzerBeep(startup_freq, 100);
        startup_start_time = HAL_GetTick();
        startup_freq += 100;
    } else if (startup_freq <= 6000) {
        // 检查是否需要切换到下一个频率
        if ((HAL_GetTick() - startup_start_time) >= 100) {
            buzzerBeep(startup_freq, 100);
            startup_start_time = HAL_GetTick();
            startup_freq += 100;
        }
    } else {
        // 启动音序列完成
        startup_freq = 3000;
    }
}

// 原始蜂鸣器函数（保持兼容性）
void Buzzer(void)
{
    buzzerShortBeep(); // 默认使用短促提示音
}