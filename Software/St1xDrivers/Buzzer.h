#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

// 蜂鸣器基础发声函数
void buzzerBeep(uint16_t frequency, uint16_t duration_ms);

// 5种不同的蜂鸣器声音效果
void buzzerShortBeep(void);      // 短促提示音
void buzzerConfirmBeep(void);    // 确认音
void buzzerErrorBeep(void);      // 错误提示音
void buzzerWarningBeep(void);    // 警告音
void buzzerStartupBeep(void);    // 启动音

// 原始蜂鸣器函数（保持兼容性）
void Buzzer(void);

#endif /* __BUZZER_H */