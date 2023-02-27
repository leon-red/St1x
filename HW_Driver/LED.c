//
// Created by Leon on 2023/2/18.
//

#include "LED.h"
#include "tim.h"
#include "stm32f1xx_hal_tim.h"

/************************LED²âÊÔ***************************/
void LED_Init() {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
}

//ºìÉ«LED½¥ÁÁ½¥Ãð
void LED_RED() {
    for (uint16_t i = 1; i <= 1000; i++) {
                __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, i);
    }
    HAL_Delay(1);
    for (uint16_t i = 1000; i >= 1; i--) {
                __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, i);
    }
}

//ÂÌÉ«LED½¥ÁÁ½¥Ãð
void LED_GREEN() {
    for (
            uint16_t i = 1;
            i <= 1000; i++) {
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, i);
        HAL_Delay(1);
    }
    for (
            uint16_t i = 1000;
            i >= 1; i--) {
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, i);
        HAL_Delay(1);
    }
}

//À¶É«LED½¥ÁÁ½¥Ãð
void LED_BLUE() {
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
}

void BEEP_Task(void) {
    static uint32_t count = 0;
    static uint8_t step = 0;

    if (flg) {
        switch (step) {
            case 0: {
                HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_4);
                HAL_Delay(1);
                __HAL_TIM_SetCompare(&htim2,TIM_CHANNEL_4,20);
//                HAL_Delay(10);
//                HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
                step = 1;
            }
                break;

            case 1: {
                count++;
                if (count >= 8000) {
                    count = 0;
                    HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_4);
//                    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
                    step = 0;
                    flg = 0;
                }
            }
                break;
        }
    }
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
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET); //ÃðLED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_SET); //ÃðLED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_SET); //ÃðLED3
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
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET); //ÁÁLED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET); //ÁÁLED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_RESET); //ÁÁLED3
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
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET); //ÃðLED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_SET); //ÃðLED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_SET); //ÃðLED3
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
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET); //ÁÁLED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET); //ÁÁLED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_RESET); //ÁÁLED3
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
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET); //ÃðLED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_SET); //ÃðLED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_SET); //ÃðLED3
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
                        HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET); //ÁÁLED1
                        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET); //ÁÁLED2
                        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_RESET); //ÁÁLED3
                        step=0;

                    }
                }
                    break;
            }
        }
            break;
    }
}

/************************LED²âÊÔ***************************/