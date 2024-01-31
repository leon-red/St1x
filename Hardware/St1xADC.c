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
//    HAL_ADCEx_Calibration_Start(&hadc1);  //ADC����У׼
//    HAL_ADC_PollForConversion(&hadc1, 50); //�ȴ��ɼ�����
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *) &DMA_ADC, 1000);

    //��ֵ�˲�
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
/* �����ѹ���㹫ʽ Vin * R2/R1+R2 = Vout
* ���������ѹ��ADC * VRef / 12bit / R2 / R1 + R2 = Vin
* R1=56K(��Vin�ĵ���),R2=10K��GND����), R2 / R1 + R2
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