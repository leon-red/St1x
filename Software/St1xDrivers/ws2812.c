//
// Created by leonm on 2023/4/26.
//
#include "ws2812.h"
#include "main.h"

struct RGB_24bits RGB;

/*写入数据时序*/
void Send_A_bit(unsigned char VAL) {
    if (VAL != 1) {
        LED_RED_GPIO_Port->BSRR = LED_RGB_Pin;
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        LED_RED_GPIO_Port->BSRR = (uint32_t) LED_RGB_Pin << 16u;

        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    } else {
        LED_RED_GPIO_Port->BSRR = LED_RGB_Pin;
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        LED_RED_GPIO_Port->BSRR = (uint32_t) LED_RGB_Pin << 16u;

        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }
}

void Reset_LED() {
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RGB_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RGB_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);

}


/*发送24位字符（包含RGB信息各8位）*/
void Send_24bits(struct RGB_24bits RGB_VAL) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        Send_A_bit(RGB_VAL.G_VAL >> (7 - i) & 0x01);//注意是从高位先发
    }
    for (i = 8; i < 16; i++) {
        Send_A_bit(RGB_VAL.R_VAL >> (15 - i) & 0x01);
    }
    for (i = 16; i < 24; i++) {
        Send_A_bit(RGB_VAL.B_VAL >> (23 - i) & 0x01);
    }

}

void Test_ws2812(void) {
    RGB.G_VAL = 20;
    RGB.R_VAL = 20;
    RGB.B_VAL = 20;

    Reset_LED();

    Send_24bits(RGB);
}

int i = 0;
char flag = 0;

void WS2812_Test2(void) {
    RGB.G_VAL = 0;
    RGB.R_VAL = 0;
    RGB.B_VAL = 0;
    i++;
    if (flag == 0) {
        RGB.B_VAL = 255 - i;
        RGB.R_VAL = i;
        if (i == 255) {
            i = 0;
            flag = 1;
        }
    } else if (flag == 1) {
        RGB.R_VAL = 255 - i;
        RGB.G_VAL = i;
        if (i == 255) {
            i = 0;
            flag = 2;
        }
    } else if (flag == 2) {
        RGB.G_VAL = 255 - i;
        RGB.B_VAL = i;
        if (i == 255) {
            i = 0;
            flag = 0;
        }
    }
    Reset_LED();
    Send_24bits(RGB);
}

void Show_All_Colors() {
    struct RGB_24bits color;
    for (int r = 0; r < 255; r++) {
        for (int g = 0; g < 255; g++) {
            for (int b = 0; b < 255; b++) {
                color.R_VAL = r;
                color.G_VAL = g;
                color.B_VAL = b;
                Send_24bits(color);
            }
        }
    }
}
