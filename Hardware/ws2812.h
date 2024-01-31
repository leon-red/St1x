//
// Created by leonm on 2023/4/26.
//

#ifndef _WS2812_H
#define _WS2812_H
#include "main.h"

#define WS2812B_Pin_H HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,1);
#define WS2812B_Pin_L HAL_GPIO_WritePin(LED_RGB_GPIO_Port,LED_RGB_Pin,0);

void ws2812_onepixel(unsigned int color);
void ws281x_delay(__IO uint32_t nCount);
void ws281x_sendLow(void);
void ws281x_sendHigh(void);
void ws2811_Reset(void);
void ws281x_sendOne(uint32_t dat);
void RGB_WriteByte(uint8_t in_data);
void RGB_ColorSet(uint8_t red,uint8_t green,uint8_t blue);
void send_code(uint8_t * sdata);
void colorset(uint8_t i);
void circular_led_show();
void ws_r(void);

void ws2812_rgb(uint8_t ws_i,uint8_t ws_r,uint8_t ws_g,uint8_t ws_b);
void ws2812_refresh(uint8_t ws_i);
void ws2812_reset(uint8_t ws_set);
void send_0(void);
void send_1(void);
void send_res(void);



#endif