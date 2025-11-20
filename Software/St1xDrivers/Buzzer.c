//
// Created by leonm on 2023/2/26.
//

#include "Buzzer.h"
#include "main.h"

// 系统时钟频率（根据实际配置：HSE 8MHz * PLL 9倍 = 72MHz）
#define SYSTEM_CORE_CLOCK 72000000

// 基于CPU指令周期的微秒延迟函数
static void delay_us(uint32_t us)
{
    // 计算需要的循环次数
    // 每个循环大约需要4个时钟周期（NOP + 比较 + 跳转）
    uint32_t cycles = (us * (SYSTEM_CORE_CLOCK / 1000000)) / 4;
    
    // 使用软件循环实现精确延迟
    // 使用volatile防止编译器优化
    volatile uint32_t i;
    for (i = 0; i < cycles; i++) {
        __NOP(); // 无操作指令，占用1个时钟周期
    }
}

// 非阻塞蜂鸣器状态机相关变量
static uint8_t buzzer_state = 0; // 0: 空闲, 1: 高电平延时中, 2: 低电平延时中
static uint16_t buzzer_frequency = 0;
static uint16_t buzzer_duration_ms = 0;
static uint32_t buzzer_cycles_remaining = 0;
static uint16_t buzzer_half_period = 0;
static uint32_t buzzer_delay_start = 0;


uint16_t startup_melody[] = {300, 500, 800, 1200, 1800, 2500, 3200, 4000, 4800, 5500,6000,7000, 8000, 9000, 10000,0};

// 初始化蜂鸣器状态机
void buzzer_init(void)
{
    buzzer_state = 0;
    buzzer_frequency = 0;
    buzzer_duration_ms = 0;
    buzzer_cycles_remaining = 0;
    buzzer_half_period = 0;
    buzzer_delay_start = 0;
}

// 更新蜂鸣器状态机（需要在主循环中定期调用）
void buzzerBeep_update(void)
{
    if (buzzer_state == 0) return; // 空闲状态
    
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_us = (current_time - buzzer_delay_start) * 1000; // 转换为微秒
    
    if (elapsed_us >= buzzer_half_period) {
        if (buzzer_state == 1) { // 高电平延时完成
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
            buzzer_cycles_remaining--;
            
            if (buzzer_cycles_remaining > 0) {
                buzzer_delay_start = current_time;
                buzzer_state = 2; // 低电平延时中
            } else {
                buzzer_state = 0; // 发声完成
            }
        } else if (buzzer_state == 2) { // 低电平延时完成
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
            buzzer_delay_start = current_time;
            buzzer_state = 1; // 高电平延时中
        }
    }
}



// 改进的阻塞式蜂鸣器发声函数（完全基于微秒延时）
static void buzzer_blocking_beep(uint16_t frequency, uint16_t duration_ms)
{
    if (frequency == 0) return; // 防止除零错误
    
    uint32_t start_time = HAL_GetTick();
    uint32_t half_period_us = 500000 / frequency; // 半周期（微秒）
    
    // 完全基于微秒延时的精确频率控制
    while((HAL_GetTick() - start_time) < duration_ms) {
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
        delay_us(half_period_us);
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
        delay_us(half_period_us);
    }
}

// 1. 短促提示音
void buzzerShortBeep(void)
{
    buzzer_blocking_beep(8000, 10);
}

// 2. 确认音
void buzzerConfirmBeep(void)
{
    buzzer_blocking_beep(3000, 10);
}

// 3. 错误提示音
void buzzerErrorBeep(void)
{
    buzzer_blocking_beep(15000, 15);
}

// 4. 警告音
void buzzerWarningBeep(void)
{
    buzzer_blocking_beep(5000, 20);
}

// 5. 开机提示音（播放频率数组中的完整音符序列）
void Beep_melody(uint16_t *hz, uint16_t t)
{
    // 播放频率数组中的完整音符序列
    uint8_t i = 0;
    while(hz[i] != 0) {
        // 使用阻塞式播放，确保每个音符完整播放
        buzzer_blocking_beep(hz[i], t);
        // 添加音符间隔，让每个音符更清晰可辨
//        if (hz[i+1] != 0) { // 如果不是最后一个音符
//            HAL_Delay(30); // 30ms间隔
//        }
        i++;
    }
}



void buzzerStartupBeep(void)
{
    Beep_melody(startup_melody, 50);
}

// 6. 扫频函数（从起始频率平滑过渡到结束频率）
void buzzerSweep(uint16_t start_freq, uint16_t end_freq, uint16_t duration_ms, uint8_t direction)
{
    if (start_freq == 0 || end_freq == 0) return;
    
    uint32_t start_time = HAL_GetTick();
    uint32_t elapsed_time = 0;
    
    // 计算频率变化步长
    int16_t freq_step;
    uint16_t current_freq;
    uint16_t step_count = duration_ms / 10; // 每10ms变化一次频率
    
    if (direction == 0) { // 向上扫频
        freq_step = (end_freq - start_freq) / step_count;
        current_freq = start_freq;
    } else { // 向下扫频
        freq_step = (start_freq - end_freq) / step_count;
        current_freq = end_freq;
    }
    
    // 确保步长不为0
    if (freq_step == 0) {
        freq_step = (direction == 0) ? 1 : -1;
    }
    
    // 执行扫频
    while (elapsed_time < duration_ms) {
        // 播放当前频率的短脉冲
        buzzer_blocking_beep(current_freq, 8); // 每个频率播放8ms
        
        // 更新频率
        if (direction == 0) { // 向上扫频
            current_freq += freq_step;
            if (current_freq > end_freq) current_freq = end_freq;
        } else { // 向下扫频
            current_freq -= freq_step;
            if (current_freq < start_freq) current_freq = start_freq;
        }
        
        // 更新已用时间
        elapsed_time = HAL_GetTick() - start_time;
    }
    
    // 播放结束频率的确认音
    buzzer_blocking_beep(end_freq, 20);
}

// 7. 警报音（使用扫频效果）
void buzzerAlarmBeep(void)
{
    // 快速向上扫频，然后向下扫频，产生警报效果
    buzzerSweep(500, 3000, 200, 0);  // 向上扫频：500Hz -> 3000Hz，200ms
    HAL_Delay(50);
    buzzerSweep(3000, 500, 300, 1);  // 向下扫频：3000Hz -> 500Hz，300ms
}

// 8. 雷达扫描音效
void buzzerRadarBeep(void)
{
    // 模拟雷达扫描效果：快速扫频后短暂停顿
    buzzerSweep(1000, 8000, 300, 0);  // 快速向上扫频
    HAL_Delay(100);
    buzzerSweep(8000, 1000, 400, 1);  // 较慢向下扫频
}

// 9. 渐进启动音
void buzzerRampUpBeep(void)
{
    // 从低频逐渐升高到高频，模拟设备启动
    buzzerSweep(200, 8000, 500, 0);
}

// 10. 渐进关闭音
void buzzerRampDownBeep(void)
{
    // 从高频逐渐降低到低频，模拟设备关闭
    buzzerSweep(2000, 200, 500, 1);
}