//
// Created by leonm on 2023/2/13.
//

#include "St1xADC.h"
#include "stdio.h"
#include "u8g2.h"
#include "u8g2_oled.h"
#include "oled.h"

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
char usb[80];
char pen[80];
void DMA_ADC_TEST() {
//    HAL_ADCEx_Calibration_Start(&hadc1);//ADC采样校准
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *) &DMA_ADC, 2);
    printf("PEN=%f\r\nUSB=%f\r\n", DMA_ADC[0] * 3.3 / 4096, DMA_ADC[1] * 3.3 / 4096 );
    printf("PEN=%d\r\nUSB=%d\r\n", DMA_ADC[0], DMA_ADC[1]);
//    HAL_Delay(800);
    printf("/************************************/\r\n");
    sprintf(usb,"USB=%f",DMA_ADC[1] * 3.3 / 4096 );
    OLED_ShowString(0,0,usb,16,1);
    sprintf(usb,"USB=%d",DMA_ADC[1]);
    OLED_ShowString(0,16,usb,16,1);
    sprintf(pen,"PEN=%f",DMA_ADC[0] * 3.3 / 4096);
    OLED_ShowString(0,32,pen,16,1);
    sprintf(pen,"PEN=%d",DMA_ADC[0]);
    OLED_ShowString(0,48,pen,16,1);
    OLED_Refresh();
    HAL_Delay(1);
}