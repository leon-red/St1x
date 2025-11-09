#ifndef ST1XSTATIC_H_
#define ST1XSTATIC_H_

#include "main.h"
#include "u8g2.h"

// 函数声明
void St1xStatic_Init(void);
void St1xStatic_Action(void);
void St1xStatic_DisplayData(u8g2_t* u8g2);
void St1xStatic_SetStandbyParameters(uint32_t time_to_reduce_temp, uint32_t time_to_turn_off, float reduced_temp);
void St1xStatic_SetDefaultStandbyParameters(void);

// 手动停止标记相关函数
void St1xStatic_SetManuallyStopped(uint8_t stopped);
uint8_t St1xStatic_IsManuallyStopped(void);

// 调试函数，用于获取当前静置状态信息
uint32_t St1xStatic_GetStandbyDuration(void);
float St1xStatic_GetTotalAcceleration(void);
uint8_t St1xStatic_IsInStandbyMode(void);
uint8_t St1xStatic_IsInSleepMode(void);

// 调试信息显示函数
void St1xStatic_DisplayDebugInfo(u8g2_t* u8g2);

// 静置状态下的屏幕和LED控制函数
void St1xStatic_StandbyDisplayControl(u8g2_t* u8g2);
void St1xStatic_SetGlobalU8g2(u8g2_t* u8g2);
void St1xStatic_TimerCallback(void);

// LED呼吸效果函数
void LED_BreathingEffect(uint8_t color_mode);

#endif /* ST1XSTATIC_H_ */