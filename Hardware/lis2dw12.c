#include "lis2dw12.h"
#include "main.h"
#include "spi.h"

void LIS2DW12_SPI_Read(uint8_t addr, uint8_t* data, uint16_t len) {
    uint8_t cmd = 0x80 | addr;  // 设置读取命令地址格式：第7位为1，表示读取操作；后面7位是寄存器地址
    // 片选使能
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_RESET);
    // 发送读取命令
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    // 读取数据
    HAL_SPI_Receive(&hspi2, data, len, HAL_MAX_DELAY);
    // 片选禁能
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_SET);
}

void LIS2DW12_SPI_Write(uint8_t addr, uint8_t* data, uint16_t len) {
    uint8_t cmd = addr & 0x7F;  // 设置写入命令地址格式：将第7位设置为0，其他位保持不变
    // 片选使能
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_RESET);
    // 发送写入命令
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    // 发送数据
    HAL_SPI_Transmit(&hspi2, data, len, HAL_MAX_DELAY);
    // 片选禁能
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_SET);
}
