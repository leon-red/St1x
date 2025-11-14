// 蜂鸣器控制模块头文件
// 提供5种预设提示音和自定义频率功能

#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

// 蜂鸣器初始化函数
void buzzerInit(void);

// 非阻塞式蜂鸣器处理函数（需要在主循环中定期调用）
void buzzerProcess(void);

// 蜂鸣器基础发声函数（非阻塞式）
void buzzerBeep(uint16_t frequency, uint16_t duration_ms);

// 5种不同的蜂鸣器声音效果（非阻塞式）
void buzzerShortBeep(void);      // 短促提示音 (4000Hz, 300ms)
void buzzerConfirmBeep(void);    // 确认音 (3500Hz, 500ms)
void buzzerErrorBeep(void);      // 错误提示音 (3000Hz, 800ms)
void buzzerWarningBeep(void);    // 警告音 (5000Hz, 400ms×2次)
void buzzerStartupBeep(void);    // 启动音 (从3000Hz到6000Hz，每100Hz播放100ms)

// 原始蜂鸣器函数（保持兼容性）
void Buzzer(void);

#endif /* __BUZZER_H */