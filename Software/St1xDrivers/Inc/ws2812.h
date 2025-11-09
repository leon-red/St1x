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


#endif