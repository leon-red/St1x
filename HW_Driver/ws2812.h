//
// Created by leonm on 2023/4/26.
//

#ifndef _WS2812_H
#define _WS2812_H

#include "main.h"

void ws281x_delay(unsigned int delay_num);
void ws281x_sendLow(void);
void ws281x_sendHigh(void);
void ws2811_Reset(void);
void ws281x_sendOne(uint32_t dat);

#endif