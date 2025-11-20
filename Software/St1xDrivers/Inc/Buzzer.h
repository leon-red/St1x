#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

// 非阻塞蜂鸣器状态机函数
void buzzer_init(void);
void buzzerBeep_update(void);



// 5种不同的蜂鸣器声音效果
void buzzerShortBeep(void);      // 短促提示音
void buzzerConfirmBeep(void);    // 确认音
void buzzerErrorBeep(void);      // 错误提示音
void buzzerWarningBeep(void);    // 警告音
void Beep_melody(uint16_t *hz, uint16_t t);    // 播放旋律
void buzzerStartupBeep(void);    // 开机提示音

// 扫频处理函数
void buzzerSweep(uint16_t start_freq, uint16_t end_freq, uint16_t duration_ms, uint8_t direction); // 扫频函数
void buzzerAlarmBeep(void);      // 警报音
void buzzerRadarBeep(void);      // 雷达扫描音效
void buzzerRampUpBeep(void);     // 渐进启动音
void buzzerRampDownBeep(void);   // 渐进关闭音

#endif /* __BUZZER_H */