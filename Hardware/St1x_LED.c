//
// Created by Leon on 2023/2/18.
//

#include "St1x_LED.h"
#include "tim.h"
#include "stdio.h"

/************************LED测试***************************/
void LED_Init() {
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
            __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 0);//红色LED灯
            __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, 0);//绿色LED灯
            __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 0);//蓝色LED灯
}

//红色LED渐亮渐灭
void LED_RED() {
    for (uint16_t i = 1; i <= 1000; i++) {
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, i);
    }
    HAL_Delay(1);
    for (uint16_t i = 1000; i >= 1; i--) {
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, i);
    }
}

//绿色LED渐亮渐灭
void LED_GREEN() {
    for (
            uint16_t i = 1;
            i <= 1000; i++) {
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, i);
        HAL_Delay(1);
    }
    for (
            uint16_t i = 1000;
            i >= 1; i--) {
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, i);
        HAL_Delay(1);
    }
}

//蓝色LED渐亮渐灭
void LED_BLUE() {
    for (
            uint16_t i = 1;
            i <= 1000; i++) {
                __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, i);
        HAL_Delay(1);
    }
    for (
            uint16_t i = 1000;
            i >= 1; i--) {
                __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, i);
        HAL_Delay(1);
    }
}


uint8_t flg = 0;
uint8_t style = 0;

//static void KEY_Task(void);
//
//static void BEEP_Task(void);
//
//static void LED_Task(void);

void KEY_Task(void) {
    static uint32_t count = 0;
    static uint8_t step = 0;

    switch (step) {
        case 0: {
            if (HAL_GPIO_ReadPin(KEY_MODE_GPIO_Port, KEY_MODE_Pin) == GPIO_PIN_RESET) {
                count++;
                if (count >= 10000) {
                    count = 0;
                    step = 1;
                }
            } else { count = 0; }
        }
            break;

        case 1: {
            if (HAL_GPIO_ReadPin(KEY_MODE_GPIO_Port, KEY_MODE_Pin) == GPIO_PIN_SET) {
                style++;
                style = style % 3;
                step = 0;
                flg = 1;
            }
        }
            break;
    }
    printf("KEY事件\r\n");
}

void LED_Task(void)
{
    static uint32_t count=0;
    static uint8_t step=0;

    switch(style)
    {
        case 0:
        {
            switch(step)
            {
                case 0:
                {
                    count++;
                    if(count>=50000)
                    {
                        count=0;
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET); //灭LED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_SET); //灭LED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_SET); //灭LED3
                        step=1;
                    }
                }
                    break;

                case 1:
                {
                    count++;
                    if(count>=50000)
                    {
                        count=0;
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET); //亮LED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET); //亮LED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_RESET); //亮LED3
                        step=0;
                    }
                }
                    break;
            }
        }
            break;

        case 1:
        {
            switch(step)
            {
                case 0:
                {
                    count++;
                    if(count>=150000)
                    {
                        count=0;
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET); //灭LED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_SET); //灭LED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_SET); //灭LED3
                        step=1;
                    }
                }
                    break;

                case 1:
                {
                    count++;
                    if(count>=150000)
                    {
                        count=0;
                        HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_4);
                                __HAL_TIM_SetCompare(&htim2,TIM_CHANNEL_4,100);
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET); //亮LED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET); //亮LED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_RESET); //亮LED3
                        step=0;
                    }
                }
                    break;
            }
        }
            break;

        case 2:
        {
            switch(step)
            {
                case 0:
                {
                    count++;
                    if(count>=450000)
                    {
                        count=0;
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET); //灭LED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_SET); //灭LED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_SET); //灭LED3
                        step=1;
                    }
                }
                    break;

                case 1:
                {
                    count++;
                    if(count>=450000)
                    {
                        count=0;
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET); //亮LED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET); //亮LED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_RESET); //亮LED3
                        step=0;

                    }
                }
                    break;
            }
        }
            break;
    }
    printf("LED事件\r\n");
}

/************************LED测试***************************/
uint16_t pwmVal=0;   //PWM占空比
//uint8_t dir=1;
void pwm_up()
{
    HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_3);
    while (pwmVal< 1000)
    {
        pwmVal=pwmVal+100;
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, pwmVal);    //修改比较值，修改占空比
        HAL_Delay(1);
    }
}

void pwm_down()
{
    HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_3);
    pwmVal=pwmVal-100;
            __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, pwmVal);    //修改比较值，修改占空比
    HAL_Delay(1);
}