//
// Created by leonm on 2023/2/26.
//

#include "Buzzer.h"
#include "main.h"
#include "tim.h"

#define DLY_TIM_Handle (&htim1) //占用TIM1输出微秒延时函数
void delay_us(uint16_t nus)
{
	__HAL_TIM_SET_COUNTER(DLY_TIM_Handle, 0);
	__HAL_TIM_ENABLE(DLY_TIM_Handle);
	while (__HAL_TIM_GET_COUNTER(DLY_TIM_Handle) < nus)
	{
	}
	__HAL_TIM_DISABLE(DLY_TIM_Handle);
}

//void delay_us(uint32_t us)  //微秒延时函数
//{
//    uint32_t delay = (HAL_RCC_GetHCLKFreq() / 4000000 * us);
//    while (delay--)
//    {
//        ;
//    }
//}

void Buzzer() {
        for (uint8_t i = 0; i<255;i++) {
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
            delay_us(125);
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
            delay_us(125);
        }
}