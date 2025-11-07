#ifndef ST1XFLASH_H
#define ST1XFLASH_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

// 校准数据存储结构体
typedef struct {
    float calibration_values[9];    // 9个校准点的校准值
    uint32_t magic_number;         // 魔术数字，用于验证数据有效性
    uint32_t checksum;             // 校验和
} CalibrationData;

// 魔术数字定义
#define CALIBRATION_MAGIC_NUMBER 0xCA11B1A5  // "CALIBIAS" 的十六进制表示

// Flash存储地址定义（使用Flash的最后几页）
// STM32F103CB有128KB Flash，每页1KB，共128页
#define CALIBRATION_FLASH_ADDRESS 0x0801FC00  // 最后一页的起始地址

// 函数声明
uint8_t St1xFlash_SaveCalibrationData(const float* calibration_values, uint8_t count);
uint8_t St1xFlash_LoadCalibrationData(float* calibration_values, uint8_t count);
uint8_t St1xFlash_IsCalibrationDataValid(void);
void St1xFlash_EraseCalibrationData(void);

#endif // ST1XFLASH_H