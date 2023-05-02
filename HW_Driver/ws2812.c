//
// Created by leonm on 2023/4/26.
//
#include "WS2812.h"
#include "main.h"

/**
* @brief  ws281x模块用到的延时函数
* @param  delay_num :延时数 （示波器测量延时时间 = delay_num * 440ns ）
* @retval None
*/
void ws281x_delay(unsigned int delay_num)
{
    while (delay_num--)
        __NOP();
}


/**
* @brief  根据WS281x芯片时序图编写的发送0码，1码RESET码的函数
* @param
* @retval None
*/
void ws281x_sendLow(void)   //发送0码
{
    WS2812B_Pin_H;
    ws281x_delay(22);    //示波器测试约为440ns
    WS2812B_Pin_L;
    ws281x_delay(66);
}

void ws281x_sendHigh(void)   //发送1码
{
    WS2812B_Pin_H;
    ws281x_delay(66);
    WS2812B_Pin_L;
    ws281x_delay(22);
}

void ws2811_Reset(void)        //发送RESET码
{
    WS2812B_Pin_L;
    ws281x_delay(60);
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

void RGB_WriteByte(uint8_t in_data){
    uint8_t n = 0;
    uint8_t y = 0,z = 0;
    n = in_data;
    for(y = 0;y < 8;y++){
        z = ((n<<y)&0x80);
        if(z){
            ws281x_sendHigh();
        }else{
            ws281x_sendLow();
        }
    }
}

uint8_t rgb_data[24] = {0x00, 0x00, 0x00,
                        0x00, 0xFF, 0x00,
                        0xFF, 0x00, 0x00,
                        0x00, 0x00, 0xFF,
                        0xFF, 0xFF, 0x00,
                        0xFF,0x00, 0xFF,
                        0xFF, 0xFF, 0xFF,
                        0X00, 0XFF, 0XFF};

//分三次发送（注意是RGB，该函数改变了GRB这个难受的发送方式）
// 设置一个灯的颜色
void RGB_ColorSet(uint8_t red,uint8_t green,uint8_t blue)
{
    // 灯的实际写入颜色是GRB
    RGB_WriteByte(green);  // 写入绿色
    RGB_WriteByte(red); // 写入红色
    RGB_WriteByte(blue); // 写入蓝色
}

//RGB解算并发送
void send_code(uint8_t *sdata) {
    uint8_t n = 0, j = 0;
    uint8_t x = 0, y = 0, z = 0;
//    for (j = 0; j < led_number ; j++)
//    {
        for (x = 0; x < 3; x++) {
            n = sdata[x];
            for (y = 0; y < 8; y++) {
                z = ((n << y) & 0x80);
                if (z) {
                    WS2812B_Pin_H;
                    ws281x_delay(7);
                    WS2812B_Pin_L;
                    ws281x_delay(1);
                } else {
                    WS2812B_Pin_H;
                    ws281x_delay(1);
                    WS2812B_Pin_L;
                    ws281x_delay(7);
                }
            }
//        }
    }
}

//自定义颜色
void colorset(uint8_t i)
{
    switch(i)
    {
        case 0:
            send_code(&rgb_data[0]);//所有灯清零
            break;
        case 1:
            send_code(&rgb_data[3]);//所有灯变红灯
            break;
        case 2:
            send_code(&rgb_data[6]);//所有灯变绿灯
            break;
        case 3:
            send_code(&rgb_data[9]);//所有灯变蓝灯
            break;
        case 4:
            send_code(&rgb_data[13]);//所有灯变红灯//所有灯变绿灯
            break;
        case 5:
            send_code(&rgb_data[22]);//所有灯变红灯//所有灯变蓝灯
            break;
        case 6:
            send_code(&rgb_data[19]);//所有灯变红灯//所有灯变绿灯//所有灯变蓝灯
            break;
        case 7:
            send_code(&rgb_data[16]);//所有灯变绿灯//所有灯变蓝灯
            break;
    }
}

// 循环显示各种彩色
void circular_led_show() {
    uint8_t i = 0, j = 0;
    for (i = 0; i < 8; i++) {
        colorset(i);
        HAL_Delay(300);
    }
}


uint8_t ws_data[200] = {0};

void ws2812_rgb(uint8_t ws_i, uint8_t ws_r, uint8_t ws_g, uint8_t ws_b) {
    ws_data[(ws_i - 1) * 3] = ws_g;
    ws_data[(ws_i - 1) * 3 + 1] = ws_r;
    ws_data[(ws_i - 1) * 3 + 2] = ws_b;
}

void ws2812_reset(uint8_t ws_set) {
    for (int i = 0; i < 200; i++) { ws_data[i] = ws_set; }
}

void send_0(void)   //发送0码
{
    WS2812B_Pin_H;
    ws281x_delay(1);    //示波器测试约为440ns
    WS2812B_Pin_L;
    ws281x_delay(2);
}

void send_1(void)   //发送1码
{
    WS2812B_Pin_H;
    ws281x_delay(2);
    WS2812B_Pin_L;
    ws281x_delay(1);
}

void send_res(void)        //发送RESET码
{
    WS2812B_Pin_L;
    ws281x_delay(680);
}

void ws2812_refresh(uint8_t ws_i) {
    uint8_t ws_ri = 0;
    for (; ws_ri < ws_i * 3; ws_ri++) {
        if ((ws_data[ws_ri] & 0x80) == 0) send_0(); else send_1();
        if ((ws_data[ws_ri] & 0x40) == 0) send_0(); else send_1();
        if ((ws_data[ws_ri] & 0x20) == 0) send_0(); else send_1();
        if ((ws_data[ws_ri] & 0x10) == 0) send_0(); else send_1();
        if ((ws_data[ws_ri] & 0x08) == 0) send_0(); else send_1();
        if ((ws_data[ws_ri] & 0x04) == 0) send_0(); else send_1();
        if ((ws_data[ws_ri] & 0x02) == 0) send_0(); else send_1();
        if ((ws_data[ws_ri] & 0x01) == 0) send_0(); else send_1();
    }
    send_res();
}