//
// Created by leonm on 2024/3/25.
//

#include "main.h"

#ifndef ST1X_LIS2DW12_H
#define ST1X_LIS2DW12_H

void LIS2DW12_SPI_Read(uint8_t addr, uint8_t* data, uint16_t len);
void LIS2DW12_SPI_Write(uint8_t addr, uint8_t* data, uint16_t len);
void lis2dw12_read_data_polling(void);
#endif //ST1X_LIS2DW12_H
