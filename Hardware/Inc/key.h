//
// Created by leonm on 2023/2/12.
//

#ifndef __KEY_H
#define __KEY_H
#include "stdio.h"
#include "u8g2.h"

extern u8g2_t u8g2;

void UP_PRESS_DOWN_Handler(void *btn);
void UP_PRESS_UP_Handler(void *btn);
void UP_PRESS_REPEAT_Handler(void *btn);
void UP_SINGLE_Click_Handler(void *btn);
void UP_DOUBLE_Click_Handler(void *btn);
void UP_LONG_PRESS_START_Handler(void *btn);
void UP_LONG_PRESS_HOLD_Handler(void *btn);
void KEY_Init();
void Iron_PullUp();

#endif
