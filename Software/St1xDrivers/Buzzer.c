//
// Created by leonm on 2023/2/26.
//

#include "Buzzer.h"
#include "main.h"
#include "tim.h"

#define DLY_TIM_Handle (&htim1) // 使用TIM1作为微秒延时定时器

// 微秒延时函数
void delay_us(uint16_t nus)
{
	__HAL_TIM_SET_COUNTER(DLY_TIM_Handle, 0);
	__HAL_TIM_ENABLE(DLY_TIM_Handle);
	while (__HAL_TIM_GET_COUNTER(DLY_TIM_Handle) < nus)
	{
	}
	__HAL_TIM_DISABLE(DLY_TIM_Handle);
}

// 蜂鸣器发声函数
void buzzerBeep(uint16_t frequency, uint16_t duration_ms)
{
    uint16_t half_period = 500000 / frequency; // 计算半周期（微秒）
    uint32_t cycles = (duration_ms * 1000) / (half_period * 2); // 计算周期数
    
    for (uint32_t i = 0; i < cycles; i++) {
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
        delay_us(half_period);
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
        delay_us(half_period);
    }
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
    for (uint8_t i = 0; i < 2; i++) {
        buzzerBeep(5000, 400);
        HAL_Delay(100); // 100ms间隔
    }
}

// 5. 启动音（频率从3000Hz到6000Hz，持续1200ms）
void buzzerStartupBeep(void)
{
    // 频率从3000Hz到6000Hz，每300ms增加1000Hz
    for (uint16_t freq = 100; freq <= 10000; freq += 100) {
        buzzerBeep(freq, 100);
    }
}

// 原始蜂鸣器函数（保持兼容性）
void Buzzer(void)
{
    buzzerShortBeep(); // 默认使用短促提示音
}