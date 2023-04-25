//
// Created by leonm on 2023/4/26.
//
#include "WS2812.h"
#include "tim.h"

uint16_t send_Buf[NUM];

// ����DMA��������
void WS_Load(void)
{
    HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2, (uint32_t *)send_Buf, NUM);
}

// �ر�����LED��
void WS_CloseAll(void)
{
    uint16_t i;

    for (i = 0; i < PIXEL_NUM * 24; i++)
        send_Buf[i] = WS0; // д���߼�0��ռ�ձ�

    for (i = PIXEL_NUM * 24; i < NUM; i++)
        send_Buf[i] = 0; // ռ�ձȱ�Ϊ0��ȫΪ�͵�ƽ

    WS_Load();
}

/**
 * @brief ȫ��led�����ó�һ�������ȣ�����RGB�ֱ���������
 * WS2812��д��˳����GRB����λ��ǰ��
*/
void WS_WriteAll_RGB(uint8_t n_R, uint8_t n_G, uint8_t n_B)
{
    uint16_t i, j;
    uint8_t dat[24];

    // ��RGB���ݽ���ת��
    for (i = 0; i < 8; i++)
    {
        dat[i] = ((n_G & 0x80) ? WS1 : WS0);
        n_G <<= 1;
    }
    for (i = 0; i < 8; i++)
    {
        dat[i + 8] = ((n_R & 0x80) ? WS1 : WS0);
        n_R <<= 1;
    }
    for (i = 0; i < 8; i++)
    {
        dat[i + 16] = ((n_B & 0x80) ? WS1 : WS0);
        n_B <<= 1;
    }

    for (i = 0; i < PIXEL_NUM; i++)
    {
        for (j = 0; j < 24; j++)
        {
            send_Buf[i * 24 + j] = dat[j];
        }
    }

    for (i = PIXEL_NUM * 24; i < NUM; i++)
    {
        send_Buf[i] = 0; // ռ�ձȱ�Ϊ0��ȫΪ�͵�ƽ
    }

    WS_Load();
}

uint32_t WS281x_Color(uint8_t red, uint8_t green, uint8_t blue)
{
    return green << 16 | red << 8 | blue;
}

/**
  * @brief  Set the color of No.n light in 24bit-format .
  * @note   None
  * @param  n: No.n lamp light, ranges from 0 to (PIXEL_NUM-1)
  * @param  GRBColor: color value of No.n light in 24bit-format(8 bit G + 8 bit R +8 bit B)
  * @retval None
  */
void WS281x_SetPixelColor(uint16_t n, uint32_t GRBColor)
{
    uint8_t i;
    if (n < PIXEL_NUM)
    {
        for (i = 0; i < 24; ++i)
        {
            send_Buf[24 * n + i] = (((GRBColor << i) & 0X800000) ? WS1 : WS0);
        }
    }
}

/**
  * @brief  Set the color of No.n light in RGB-format .
  * @note   None
  * @param  n: No.n lamp light, ranges from 0 to (PIXEL_NUM-1)
  * @param  red: red color value of No.n light in RGB-format
  * @param  green: greeen color value of No.n light in RGB-format
  * @param  blue: blue color value of No.n light in RGB-format
  * @retval None
  */
void WS281x_SetPixelRGB(uint16_t n, uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t i;

    if (n < PIXEL_NUM)
    {
        for (i = 0; i < 24; ++i)
        {
            send_Buf[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
        }
    }
}

uint32_t Wheel(uint8_t WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return WS281x_Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return WS281x_Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return WS281x_Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow(uint8_t wait)
{
    uint32_t timestamp = HAL_GetTick();
    uint16_t i;
    static uint8_t j;
    static uint32_t next_time = 0;

    uint32_t flag = 0;
    if (next_time < wait)
    {
        if ((uint64_t)timestamp + wait - next_time > 0)
            flag = 1;
    }
    else if (timestamp > next_time)
    {
        flag = 1;
    }
    if (flag) // && (timestamp - next_time < wait*5))
    {
        j++;
        next_time = timestamp + wait;
        for (i = 0; i < PIXEL_NUM; i++)
        {
            WS281x_SetPixelColor(i, Wheel((i + j) & 255));
        }
    }
    WS_Load();
}

void rainbowCycle(uint8_t wait)
{
    uint32_t timestamp = HAL_GetTick();
    uint16_t i;
    static uint8_t j;
    static uint32_t next_time = 0;

    static uint8_t loop = 0;
    if (loop == 0)
        next_time = timestamp;
    loop = 1; //�״ε��ó�ʼ��

    if ((timestamp > next_time)) // && (timestamp - next_time < wait*5))
    {
        j++;
        next_time = timestamp + wait;
        for (i = 0; i < PIXEL_NUM; i++)
        {
            WS281x_SetPixelColor(i, Wheel(((i * 256 / (PIXEL_NUM)) + j) & 255));
        }
    }
    WS_Load();
}
