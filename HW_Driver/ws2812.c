//
// Created by leonm on 2023/4/26.
//
#include "WS2812.h"
#include "main.h"

/**
* @brief  ws281xģ���õ�����ʱ����
* @param  delay_num :��ʱ�� ��ʾ����������ʱʱ�� = delay_num * 440ns ��
* @retval None
*/
void ws281x_delay(unsigned int delay_num)
{
    while (delay_num--)
        __NOP();
}


/**
* @brief  ����WS281xоƬʱ��ͼ��д�ķ���0�룬1��RRESET��ĺ���
* @param
* @retval None
*/
void ws281x_sendLow(void)   //����0��
{
    WS2812B_Pin_H;
    ws281x_delay(22);    //ʾ��������ԼΪ440ns
    WS2812B_Pin_L;
    ws281x_delay(66);
}

void ws281x_sendHigh(void)   //����1��
{
    WS2812B_Pin_H;
    ws281x_delay(66);
    WS2812B_Pin_L;
    ws281x_delay(22);
}

void ws2811_Reset(void)        //����RESET��
{
    WS2812B_Pin_L;
    ws281x_delay(60);
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

//�����η��ͣ�ע����RGB���ú����ı���GRB������ܵķ��ͷ�ʽ��
// ����һ���Ƶ���ɫ
void RGB_ColorSet(uint8_t red,uint8_t green,uint8_t blue)
{
    // �Ƶ�ʵ��д����ɫ��GRB
    RGB_WriteByte(green);  // д����ɫ
    RGB_WriteByte(red); // д���ɫ
    RGB_WriteByte(blue); // д����ɫ
}

//RGB���㲢����
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

//�Զ�����ɫ
void colorset(uint8_t i)
{
    switch(i)
    {
        case 0:
            send_code(&rgb_data[0]);//���е�����
            break;
        case 1:
            send_code(&rgb_data[3]);//���еƱ���
            break;
        case 2:
            send_code(&rgb_data[6]);//���еƱ��̵�
            break;
        case 3:
            send_code(&rgb_data[9]);//���еƱ�����
            break;
        case 4:
            send_code(&rgb_data[13]);//���еƱ���//���еƱ��̵�
            break;
        case 5:
            send_code(&rgb_data[22]);//���еƱ���//���еƱ�����
            break;
        case 6:
            send_code(&rgb_data[19]);//���еƱ���//���еƱ��̵�//���еƱ�����
            break;
        case 7:
            send_code(&rgb_data[16]);//���еƱ��̵�//���еƱ�����
            break;
    }
}

// ѭ����ʾ���ֲ�ɫ
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

void send_0(void)   //����0��
{
    WS2812B_Pin_H;
    ws281x_delay(1);    //ʾ��������ԼΪ440ns
    WS2812B_Pin_L;
    ws281x_delay(2);
}

void send_1(void)   //����1��
{
    WS2812B_Pin_H;
    ws281x_delay(2);
    WS2812B_Pin_L;
    ws281x_delay(1);
}

void send_res(void)        //����RESET��
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