//
// Created by leonm on 2023/2/13.
//

#include "St1xADC.h"
#include "adc.h"
#include "stdio.h"
#include "u8g2.h"

uint16_t ADC_IN_1(void) {
//    HAL_ADC_Start(&hadc1);//开启ADC采集
    HAL_ADC_PollForConversion(&hadc1, 1000);//等待采集结束
    if (HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC))//读取ADC完成标志位
    {
        return HAL_ADC_GetValue(&hadc1);//读出ADC数值
    }
    return 0;
}

uint16_t DMA_ADC[2];
u8g2_t u8g2;

void DMA_ADC_TEST(u8g2_t *u8g2) {
//    HAL_ADCEx_Calibration_Start(&hadc1);//ADC采样校准
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *) &DMA_ADC, 2);
    printf("PEN=%f\r\nUSB=%f\r\n", DMA_ADC[0] * 3.3 / 4096, DMA_ADC[1] * 3.3 / 4096);
    printf("PEN=%d\r\nUSB=%d\r\n", DMA_ADC[0], DMA_ADC[1]);
    printf("/************************************/\r\n");

    u8g2_ClearBuffer(u8g2);
    char buff[20], iron[20];
    sprintf(buff, "USB=%.3fV", DMA_ADC[1] * 3.27 / 4096 * 6.6);
    sprintf(iron, "Iron=%.4f", DMA_ADC[0] * 3.3 / 4096);
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(u8g2, 0, 16, buff);
    u8g2_DrawStr(u8g2, 0, 48, iron);
    sprintf(buff, "USB=%d", DMA_ADC[1]);
    sprintf(iron, "Iron=%d", DMA_ADC[0]);
    u8g2_DrawStr(u8g2, 0, 32, buff);
    u8g2_DrawStr(u8g2, 0, 64, iron);
    u8g2_SendBuffer(u8g2);
}