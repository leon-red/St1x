//
// Created by leonm on 2023/2/26.
//

#include "Buzzer.h"
#include "main.h"
#include "tim.h"

void beep(uint32_t *Data) {
            HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_3,Data,1);   //·äÃùÆ÷
                    __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, *Data);   //·äÃùÆ÷
}