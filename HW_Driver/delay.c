//
// Created by leonm on 2023/2/18.
//

#include "delay.h"
#include "tim.h"
/* 微秒延时波函数 */
#define DLY_TIM_Handle &htim1
void Delay_us(uint16_t nus) //浪费一个定时器换取一个微秒延时函数
{
    __HAL_TIM_SET_COUNTER(DLY_TIM_Handle,0);
    __HAL_TIM_ENABLE(DLY_TIM_Handle);
    while (__HAL_TIM_GET_COUNTER(DLY_TIM_Handle) < nus)
    {
    }
    __HAL_TIM_DISABLE(DLY_TIM_Handle);
}