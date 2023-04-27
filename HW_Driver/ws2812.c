//
// Created by leonm on 2023/4/26.
//
#include "WS2812.h"

/**
* @brief  ws281xģ���õ�����ʱ����
* @param  delay_num :��ʱ�� ��ʾ����������ʱʱ�� = delay_num * 440ns ��
* @retval None
*/
void ws281x_delay(unsigned int delay_num)
{
    while (delay_num--);
}

/**
* @brief  ����WS281xоƬʱ��ͼ��д�ķ���0�룬1��RRESET��ĺ���
* @param
* @retval None
*/
void ws281x_sendLow(void)   //����0��
{
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,1);
    ws281x_delay(1);    //ʾ��������ԼΪ440ns
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
    ws281x_delay(2);
}
void ws281x_sendHigh(void)   //����1��
{
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,1);
    ws281x_delay(2);
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
    ws281x_delay(1);
}
void ws2811_Reset(void)        //����RESET��
{
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
    ws281x_delay(60);
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,1);
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
}

/**
* @brief  ���͵���һ���Ƶ����ݣ���24bit��
* @param  dat����ɫ��24λ����
* @retval None
*/
void ws281x_sendOne(uint32_t dat) {
    uint8_t i;
    unsigned char byte;
    for (i = 24; i > 0; i--) {
        byte = ((dat >> i) & 0x01);  //λ��������ȡdat���ݵĵ�iλ
        if (byte == 1) {
            ws281x_sendHigh();
        } else {
            ws281x_sendLow();
        }
    }
}