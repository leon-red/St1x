//
// Created by leonm on 2023/2/18.
//

#include "delay.h"
#include "tim.h"
/* ΢����ʱ������ */
#define DLY_TIM_Handle &htim1
void Delay_us(uint16_t nus) //�˷�һ����ʱ����ȡһ��΢����ʱ����
{
    __HAL_TIM_SET_COUNTER(DLY_TIM_Handle,0);
    __HAL_TIM_ENABLE(DLY_TIM_Handle);
    while (__HAL_TIM_GET_COUNTER(DLY_TIM_Handle) < nus)
    {
    }
    __HAL_TIM_DISABLE(DLY_TIM_Handle);
}