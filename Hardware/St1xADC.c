//
// Created by leonm on 2023/2/13.
//

#include "St1xADC.h"
#include "adc.h"
#include "stdio.h"
#include "u8g2.h"
#include "tim.h"



uint16_t DMA_ADC[1000]={0};

void DMA_ADC_TEST(u8g2_t *u8g2) {
//    HAL_ADCEx_Calibration_Start(&hadc1);  //ADC采样校准
//    HAL_ADC_PollForConversion(&hadc1, 50); //等待采集结束
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *) &DMA_ADC, 1000);

    //均值滤波
    uint32_t ad1_iron = 0, ad2_usb = 0;
    int i = 0;
    for (i = 0, ad1_iron = ad2_usb = 0; i < 1000;) {
        ad1_iron += DMA_ADC[i++];
        ad2_usb += DMA_ADC[i++];
    }
    ad1_iron = ad1_iron / 500;
    ad2_usb = ad2_usb / 500;

    printf("USB=%f\r\nUSB=%d\r\n", DMA_ADC[1] * 3.3 / 4096 / 0.151515, DMA_ADC[1]);
    printf("PEN=%f\r\nPEN=%d\r\n", DMA_ADC[0] * 3.3 / 4096, DMA_ADC[0]);
    printf("/************************************/\r\n");
//}
//
//void Display_ADC(u8g2_t *u8g2){
    char usb[20], iron[20];
/* 电阻分压计算公式 Vin * R2/R1+R2 = Vout
* 计算输入电压：ADC * VRef / 12bit / R2 / R1 + R2 = Vin
* R1=56K(接Vin的电阻),R2=10K接GND电阻), R2 / R1 + R2
*/
    sprintf(usb, "USB=%0.4fV", ad2_usb * 3.3 / 4096 / 0.151515);
    sprintf(iron, "Iron=%f`V", ad1_iron * 3.3 / 4096);

    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(u8g2, 0, 16, usb);
    u8g2_DrawStr(u8g2, 0, 48, iron);
    sprintf(usb, "USB=%d", DMA_ADC[1]);
    sprintf(iron, "Iron=%d", DMA_ADC[0]);
    u8g2_DrawStr(u8g2, 0, 32, usb);
    u8g2_DrawStr(u8g2, 0, 64, iron);
    u8g2_DrawStr(u8g2, 24, 80, "Enter to Test");
    u8g2_SendBuffer(u8g2);
}
