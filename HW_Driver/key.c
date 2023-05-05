//
// Created by leonm on 2023/2/12.
//��������Դ���ַ��github.com/0x1abin/MultiButton
//
//

#include "Inc/key.h"
#include "Inc/multi_button.h"
#include "main.h"
#include "stdio.h"
#include "LED.h"
#include "tim.h"
#include "St1xADC.h"

/************************�����¼�����**********************************/
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

/************************���°���ɨ���¼�**********************************/
void UP_PRESS_DOWN_Handler(void *btn) {

    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP��������\r\n");
            break;
        case MODE_id:
            printf("MODE��������\r\n");
            break;
        case DOWN_id:
            printf("DOWN��������\r\n");
            break;
        default:
            break;
    }
}
void UP_PRESS_UP_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP��������\r\n");
            break;
        case MODE_id:
            printf("MODE��������\r\n");
            break;
        case DOWN_id:
            printf("DOWN��������\r\n");
            break;
        default:
            break;
    }
}
void UP_PRESS_REPEAT_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP�ظ����´���\r\n");
            break;
        case MODE_id:
            printf("MODE�ظ����´���\r\n");
            break;
        case DOWN_id:
            printf("DOWN�ظ����´���\r\n");
            break;
        default:
            break;
    }
}
void UP_SINGLE_Click_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP��������\r\n");
            break;
        case MODE_id:
            printf("MODE��������\r\n");
            break;
        case DOWN_id:
            printf("DOWN��������\r\n");
            break;
        default:
            break;
    }
}
void UP_DOUBLE_Click_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP����˫��\r\n");
            break;
        case MODE_id:
            printf("MODE����˫��\r\n");
            break;
        case DOWN_id:
            printf("DOWN����˫��\r\n");
            break;
        default:
            break;
    }
}
void UP_LONG_PRESS_START_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP�ﵽ����ʱ����ֵʱ����һ��\r\n");
            break;
        case MODE_id:
            printf("MODE�ﵽ����ʱ����ֵʱ����һ��\r\n");
            break;
        case DOWN_id:
            printf("DOWN�ﵽ����ʱ����ֵʱ����һ��\r\n");
            break;
        default:
            break;
    }
}
void UP_LONG_PRESS_HOLD_Handler(void *btn) {
    Button *temp_button = (Button *) btn;
    switch (temp_button->button_id) {
        case UP_id:
            printf("UP�����ڼ�һֱ����\r\n");
            break;
        case MODE_id:
            printf("MODE�����ڼ�һֱ����\r\n");
            break;
        case DOWN_id:
            printf("DOWN�����ڼ�һֱ����\r\n");
            break;
        default:
            break;
    }
}
/************************���°���ɨ���¼�**********************************/

/****************��ʼ���������󣬰󶨰�����GPIO��ƽ��ȡ�ӿ�read_button_pin() ����һ������������Ч������ƽ******************/
/****************��ע�ᰴ���¼�******************/
/****************�ټӰ�������******************/
void KEY_Init() {
    button_init(&UP, read_button_GPIO, 0, UP_id);
    button_init(&MODE, read_button_GPIO, 0, MODE_id);
    button_init(&DOWN, read_button_GPIO, 0, DOWN_id);
/****************UP�����¼�Ӧ��******************/
    button_attach(&UP, PRESS_DOWN, UP_PRESS_DOWN_Handler);  //1���������£�ÿ�ΰ��¶�����
    button_attach(&UP, PRESS_UP, UP_PRESS_UP_Handler);  //2����������ÿ���ɿ�������
    button_attach(&UP, PRESS_REPEAT, UP_PRESS_REPEAT_Handler);  //3���ظ����´���������repeat������������
    button_attach(&UP, SINGLE_CLICK, UP_SINGLE_Click_Handler);  //4�����������¼�
    button_attach(&UP, DOUBLE_CLICK, UP_DOUBLE_Click_Handler);  //5��˫�������¼�
    button_attach(&UP, LONG_PRESS_START, UP_LONG_PRESS_START_Handler);    //6���ﵽ����ʱ����ֵʱ����һ��
    button_attach(&UP, LONG_PRESS_HOLD, UP_LONG_PRESS_HOLD_Handler);  //7�������ڼ�һֱ����
/****************UP�����¼�Ӧ��******************/

/****************MODE�����¼�Ӧ��******************/
    button_attach(&MODE, PRESS_DOWN, UP_PRESS_DOWN_Handler);  //1���������£�ÿ�ΰ��¶�����
    button_attach(&MODE, PRESS_UP, UP_PRESS_UP_Handler);  //2����������ÿ���ɿ�������
    button_attach(&MODE, PRESS_REPEAT, UP_PRESS_REPEAT_Handler);  //3���ظ����´���������repeat������������
    button_attach(&MODE, SINGLE_CLICK, UP_SINGLE_Click_Handler);  //4�����������¼�
    button_attach(&MODE, DOUBLE_CLICK, UP_DOUBLE_Click_Handler);  //5��˫�������¼�
    button_attach(&MODE, LONG_PRESS_START, UP_LONG_PRESS_START_Handler);    //6���ﵽ����ʱ����ֵʱ����һ��
    button_attach(&MODE, LONG_PRESS_HOLD, UP_LONG_PRESS_HOLD_Handler);  //7�������ڼ�һֱ����
/****************MODE�����¼�Ӧ��******************/

/****************DOWN�����¼�Ӧ��******************/
    button_attach(&DOWN, PRESS_DOWN, UP_PRESS_DOWN_Handler);  //1���������£�ÿ�ΰ��¶�����
    button_attach(&DOWN, PRESS_UP, UP_PRESS_UP_Handler);  //2����������ÿ���ɿ�������
    button_attach(&DOWN, PRESS_REPEAT, UP_PRESS_REPEAT_Handler);  //3���ظ����´���������repeat������������
    button_attach(&DOWN, SINGLE_CLICK, UP_SINGLE_Click_Handler);  //4�����������¼�
    button_attach(&DOWN, DOUBLE_CLICK, UP_DOUBLE_Click_Handler);  //5��˫�������¼�
    button_attach(&DOWN, LONG_PRESS_START, UP_LONG_PRESS_START_Handler);    //6���ﵽ����ʱ����ֵʱ����һ��
    button_attach(&DOWN, LONG_PRESS_HOLD, UP_LONG_PRESS_HOLD_Handler);  //7�������ڼ�һֱ����
/****************DOWN�����¼�Ӧ��******************/

/****************��������******************/
    button_start(&UP);
    button_start(&MODE);
    button_start(&DOWN);
/****************��������******************/

//make the timer invoking the button_ticks() interval 5ms.
//This function is implemented by yourself.
//    __timer_start(button_ticks, 0, 5);
}
/****************��ʼ������******************/
void delay_ms(uint32_t nms)   //����SysTickдһ����ʱ����
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

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)//�жϻص���������������������
{
    switch(GPIO_Pin)
    {
        case KEY_UP_Pin:
            pwm_up();
            break;
        case KEY_DOWN_Pin:
            pwm_down();
            break;
    }
}

void Iron_PullUp() {
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
            __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 1000);//��ɫLED��
            __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, 1000);//��ɫLED��
            __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 1);//��ɫLED��
    if (HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == 0) {
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
                __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 600);   //�����¶ȿ���
                __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 1);   //������
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, 900);   //��ɫLED
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
        u8g2_DrawStr(&u8g2, 64, 64, "Heating Up");
//        u8g2_SendBuffer(&u8g2);
    } else {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
                __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, 1000);   //��ɫLED
    }
}