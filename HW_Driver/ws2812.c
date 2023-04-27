//
// Created by leonm on 2023/4/26.
//
#include "WS2812.h"

/**
* @brief  ws281x模块用到的延时函数
* @param  delay_num :延时数 （示波器测量延时时间 = delay_num * 440ns ）
* @retval None
*/
void ws281x_delay(unsigned int delay_num)
{
    while (delay_num--);
}

/**
* @brief  根据WS281x芯片时序图编写的发送0码，1码RESET码的函数
* @param
* @retval None
*/
void ws281x_sendLow(void)   //发送0码
{
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,1);
    ws281x_delay(1);    //示波器测试约为440ns
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
    ws281x_delay(2);
}
void ws281x_sendHigh(void)   //发送1码
{
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,1);
    ws281x_delay(2);
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
    ws281x_delay(1);
}
void ws2811_Reset(void)        //发送RESET码
{
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
    ws281x_delay(60);
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,1);
    HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);
}

/**
* @brief  发送点亮一个灯的数据（即24bit）
* @param  dat：颜色的24位编码
* @retval None
*/
void ws281x_sendOne(uint32_t dat) {
    uint8_t i;
    unsigned char byte;
    for (i = 24; i > 0; i--) {
        byte = ((dat >> i) & 0x01);  //位操作，读取dat数据的第i位
        if (byte == 1) {
            ws281x_sendHigh();
        } else {
            ws281x_sendLow();
        }
    }
}