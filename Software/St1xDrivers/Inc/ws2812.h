//
// Created by leonm on 2023/4/26.
//

#ifndef _WS2812_H
#define _WS2812_H
#include "main.h"


//�洢һ��ת�����RGB2������  ��24bits
struct RGB_24bits                    //�ṹ��
{
    unsigned char G_VAL;                 //��ɫ  0-255
    unsigned char R_VAL;                  //��ɫ 0-255
    unsigned char B_VAL;                  //��ɫ0-255
};


void Send_A_bit(unsigned char VAL);
void Reset_LED();
void Send_24bits(struct RGB_24bits RGB_VAL);
void Test_ws2812(void);
void WS2812_Test2(void);
void Show_All_Colors();
void HeatingStatusLEDEffect(void);

/* HSV颜色模型相关函数 */
struct RGB_24bits HSVtoRGB(float h, float s, float v);
void HSV_RainbowEffect(void);
void HSV_BreathingEffect(uint8_t hue);

/* 智能亮度控制系统 */
void RGB_AdjustBrightness(struct RGB_24bits* color, float brightness);
void HSV_SmartBrightnessControl(float hue, float saturation, float target_brightness);
void TemperatureSmartLEDControl(float current_temp, float target_temp, uint8_t heating_status);
void FastRGB_SetColor(uint8_t r, uint8_t g, uint8_t b);
void SystemStatusLEDIndicator(uint8_t system_state);
void UnifiedLEDStateMachine(void);


#endif