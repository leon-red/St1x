//
// Created by leonm on 2023/2/13.
//

#include "Inc/St1xadc.h"
uint16_t ADC_IN_1(void){
    HAL_ADC_Start(&hadc1);//����ADC�ɼ�
    HAL_ADC_PollForConversion(&hadc1,1000);//�ȴ��ɼ�����
    if(HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1),HAL_ADC_STATE_REG_EOC))//��ȡADC��ɱ�־λ
    {
        return HAL_ADC_GetValue(&hadc1);//����ADC��ֵ
    }return 0;
}
