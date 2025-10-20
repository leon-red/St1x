//
// Created by leonm on 2023/2/12.
//按键驱动源码地址：github.com/0x1abin/MultiButton
//
//

#include "Inc/St1x_key.h"
#include "Inc/multi_button.h"
#include "main.h"
#include "stdio.h"
#include "St1x_LED.h"
#include "tim.h"
#include "St1xADC.h"
#include "Buzzer.h"

/************************按键事件测试**********************************/
enum Button_IDs {
    UP_id,
    MODE_id,
    DOWN_id,
};
struct Button UP;
struct Button MODE;
struct Button DOWN;

uint8_t read_button_GPIO(uint8_t button_id) {
    // you can share the GPIO read function with multiple Buttons
    switch (button_id) {
        case UP_id:
            return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin);
        case MODE_id:
            return HAL_GPIO_ReadPin(KEY_MODE_GPIO_Port, KEY_MODE_Pin);
        case DOWN_id:
            return HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port, KEY_DOWN_Pin);
        default:
            return 0;
    }
}

/************************按下按键扫描事件**********************************/
void UP_PRESS_DOWN_Handler(void *btn) {

    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP按键按下\r\n");
            break;
        case MODE_id:
            printf("MODE按键按下\r\n");
            break;
        case DOWN_id:
            printf("DOWN按键按下\r\n");
            break;
        default:
            break;
    }
}
void UP_PRESS_UP_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP按键弹起\r\n");
            break;
        case MODE_id:
            printf("MODE按键弹起\r\n");
            break;
        case DOWN_id:
            printf("DOWN按键弹起\r\n");
            break;
        default:
            break;
    }
}
void UP_PRESS_REPEAT_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP重复按下触发\r\n");
            break;
        case MODE_id:
            printf("MODE重复按下触发\r\n");
            break;
        case DOWN_id:
            printf("DOWN重复按下触发\r\n");
            break;
        default:
            break;
    }
}
void UP_SINGLE_Click_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP按键单击\r\n");
            break;
        case MODE_id:
            printf("MODE按键单击\r\n");
            break;
        case DOWN_id:
            printf("DOWN按键单击\r\n");
            break;
        default:
            break;
    }
}
void UP_DOUBLE_Click_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP按键双击\r\n");
            break;
        case MODE_id:
            printf("MODE按键双击\r\n");
            break;
        case DOWN_id:
            printf("DOWN按键双击\r\n");
            break;
        default:
            break;
    }
}
void UP_LONG_PRESS_START_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP达到长按时间阈值时触发一次\r\n");
            break;
        case MODE_id:
            printf("MODE达到长按时间阈值时触发一次\r\n");
            break;
        case DOWN_id:
            printf("DOWN达到长按时间阈值时触发一次\r\n");
            break;
        default:
            break;
    }
}
void UP_LONG_PRESS_HOLD_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP长按期间一直触发\r\n");
            break;
        case MODE_id:
            printf("MODE长按期间一直触发\r\n");
            break;
        case DOWN_id:
            printf("DOWN长按期间一直触发\r\n");
            break;
        default:
            break;
    }
}
/************************按下按键扫描事件**********************************/

/****************初始化按键对象，绑定按键的GPIO电平读取接口read_button_pin() ，后一个参数设置有效触发电平******************/
/****************加注册按键事件******************/
/****************再加按键启动******************/
void KEY_Init() {
    button_init(&UP, read_button_GPIO, 0, UP_id);
    button_init(&MODE, read_button_GPIO, 0, MODE_id);
    button_init(&DOWN, read_button_GPIO, 0, DOWN_id);
/****************UP按键事件应答******************/
    button_attach(&UP, PRESS_DOWN, UP_PRESS_DOWN_Handler);  //1、按键按下，每次按下都触发
    button_attach(&UP, PRESS_UP, UP_PRESS_UP_Handler);  //2、按键弹起，每次松开都触发
    button_attach(&UP, PRESS_REPEAT, UP_PRESS_REPEAT_Handler);  //3、重复按下触发，变量repeat计数连击次数
    button_attach(&UP, SINGLE_CLICK, UP_SINGLE_Click_Handler);  //4、单击按键事件
    button_attach(&UP, DOUBLE_CLICK, UP_DOUBLE_Click_Handler);  //5、双击按键事件
    button_attach(&UP, LONG_PRESS_START, UP_LONG_PRESS_START_Handler);    //6、达到长按时间阈值时触发一次
    button_attach(&UP, LONG_PRESS_HOLD, UP_LONG_PRESS_HOLD_Handler);  //7、长按期间一直触发
/****************UP按键事件应答******************/

/****************MODE按键事件应答******************/
    button_attach(&MODE, PRESS_DOWN, UP_PRESS_DOWN_Handler);  //1、按键按下，每次按下都触发
    button_attach(&MODE, PRESS_UP, UP_PRESS_UP_Handler);  //2、按键弹起，每次松开都触发
    button_attach(&MODE, PRESS_REPEAT, UP_PRESS_REPEAT_Handler);  //3、重复按下触发，变量repeat计数连击次数
    button_attach(&MODE, SINGLE_CLICK, UP_SINGLE_Click_Handler);  //4、单击按键事件
    button_attach(&MODE, DOUBLE_CLICK, UP_DOUBLE_Click_Handler);  //5、双击按键事件
    button_attach(&MODE, LONG_PRESS_START, UP_LONG_PRESS_START_Handler);    //6、达到长按时间阈值时触发一次
    button_attach(&MODE, LONG_PRESS_HOLD, UP_LONG_PRESS_HOLD_Handler);  //7、长按期间一直触发
/****************MODE按键事件应答******************/

/****************DOWN按键事件应答******************/
    button_attach(&DOWN, PRESS_DOWN, UP_PRESS_DOWN_Handler);  //1、按键按下，每次按下都触发
    button_attach(&DOWN, PRESS_UP, UP_PRESS_UP_Handler);  //2、按键弹起，每次松开都触发
    button_attach(&DOWN, PRESS_REPEAT, UP_PRESS_REPEAT_Handler);  //3、重复按下触发，变量repeat计数连击次数
    button_attach(&DOWN, SINGLE_CLICK, UP_SINGLE_Click_Handler);  //4、单击按键事件
    button_attach(&DOWN, DOUBLE_CLICK, UP_DOUBLE_Click_Handler);  //5、双击按键事件
    button_attach(&DOWN, LONG_PRESS_START, UP_LONG_PRESS_START_Handler);    //6、达到长按时间阈值时触发一次
    button_attach(&DOWN, LONG_PRESS_HOLD, UP_LONG_PRESS_HOLD_Handler);  //7、长按期间一直触发
/****************DOWN按键事件应答******************/

/****************启动按键******************/
    button_start(&UP);
    button_start(&MODE);
    button_start(&DOWN);
/****************启动按键******************/

//make the timer invoking the button_ticks() interval 5ms.
//This function is implemented by yourself.
//    __timer_start(button_ticks, 0, 5);
}
/****************初始化按键******************/
void delay_ms(uint32_t nms)   //利用SysTick写一个延时函数
{
    uint32_t temp;
    SysTick->LOAD = 8000*nms;
    SysTick->VAL=0X00;
    SysTick->CTRL=0X01;
    do
    {
        temp=SysTick->CTRL;
    }
    while((temp&0x01)&&(!(temp&(1<<16))));
    SysTick->CTRL=0x00;
    SysTick->VAL =0X00;
}

//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)//中断回调函数，当按键按下跳入
//{
//    switch(GPIO_Pin)
//    {
//        case KEY_UP_Pin:
//            pwm_up();
//            break;
//        case KEY_DOWN_Pin:
//            pwm_down();
//            break;
//    }
//}

void Iron_PullUp() {
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);   //绿色LED灯
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);   //红色LED灯
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);   //蓝色LED灯
            __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 1000);//绿色LED灯
            __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, 1000);//红色LED灯
            __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 1000);//蓝色LED灯
    if (HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == 0) {
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);   //烙铁温度控制
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);   //绿色LED
                __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 600);   //烙铁温度控制
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 950);   //绿色LED
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
        u8g2_DrawStr(&u8g2, 64, 64, "Heating Up");
//        Buzzer();   //蜂鸣器
//        u8g2_SendBuffer(&u8g2);
    } else {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);   //烙铁温度控制
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);   //蜂鸣器
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 1000);   //绿色LED
    }
}