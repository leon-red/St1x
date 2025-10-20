//
// Created by leonm on 2023/4/26.
//

#ifndef _WS2812_H
#define _WS2812_H
#include "main.h"


//存储一个转化后的RGB2进制数  共24bits
struct RGB_24bits                    //结构体
{
    unsigned char G_VAL;                 //绿色  0-255
    unsigned char R_VAL;                  //红色 0-255
    unsigned char B_VAL;                  //蓝色0-255
};


void Send_A_bit(unsigned char VAL);
void Reset_WS2812();
void Send_24bits(struct RGB_24bits RGB_VAL);
void Test_ws2812(void);
void WS2812_Test2(void);
void Show_All_Colors();


#endif