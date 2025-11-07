#include "St1xFlash.h"
#include "stm32f1xx_hal.h"
#include <string.h>

/**
 * @brief 计算校验和
 * @param data 数据指针
 * @param size 数据大小
 * @return 校验和
 */
static uint32_t calculateChecksum(const uint8_t* data, uint32_t size) {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < size; i++) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief 解锁Flash
 * @return HAL状态
 */
static HAL_StatusTypeDef unlockFlash(void) {
    return HAL_FLASH_Unlock();
}

/**
 * @brief 锁定Flash
 * @return HAL状态
 */
static HAL_StatusTypeDef lockFlash(void) {
    return HAL_FLASH_Lock();
}

/**
 * @brief 擦除Flash页
 * @param pageAddress 页地址
 * @return HAL状态
 */
static HAL_StatusTypeDef eraseFlashPage(uint32_t pageAddress) {
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError;
    
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = pageAddress;
    eraseInit.NbPages = 1;
    
    return HAL_FLASHEx_Erase(&eraseInit, &pageError);
}

/**
 * @brief 保存校准数据到Flash
 * @param calibration_values 校准值数组
 * @param count 校准值数量（应该为9）
 * @return 1=成功，0=失败
 */
uint8_t St1xFlash_SaveCalibrationData(const float* calibration_values, uint8_t count) {
    if (count != 9) {
        return 0;  // 只支持9个校准点
    }
    
    CalibrationData calData;
    
    // 填充校准数据
    for (uint8_t i = 0; i < 9; i++) {
        calData.calibration_values[i] = calibration_values[i];
    }
    
    // 设置魔术数字
    calData.magic_number = CALIBRATION_MAGIC_NUMBER;
    
    // 计算校验和
    calData.checksum = calculateChecksum((uint8_t*)calData.calibration_values, sizeof(calData.calibration_values));
    
    // 解锁Flash
    if (unlockFlash() != HAL_OK) {
        return 0;
    }
    
    // 擦除目标页
    if (eraseFlashPage(CALIBRATION_FLASH_ADDRESS) != HAL_OK) {
        lockFlash();
        return 0;
    }
    
    // 写入数据到Flash
    uint32_t* dataPtr = (uint32_t*)&calData;
    uint32_t dataSize = sizeof(CalibrationData) / sizeof(uint32_t);
    
    for (uint32_t i = 0; i < dataSize; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 
                               CALIBRATION_FLASH_ADDRESS + i * 4, 
                               dataPtr[i]) != HAL_OK) {
            lockFlash();
            return 0;
        }
    }
    
    // 锁定Flash
    lockFlash();
    
    return 1;
}

/**
 * @brief 从Flash加载校准数据
 * @param calibration_values 用于存储校准值的数组
 * @param count 校准值数量（应该为9）
 * @return 1=成功，0=失败
 */
uint8_t St1xFlash_LoadCalibrationData(float* calibration_values, uint8_t count) {
    if (count != 9) {
        return 0;  // 只支持9个校准点
    }
    
    // 检查数据是否有效
    if (!St1xFlash_IsCalibrationDataValid()) {
        return 0;
    }
    
    // 读取Flash中的数据
    CalibrationData* calData = (CalibrationData*)CALIBRATION_FLASH_ADDRESS;
    
    // 复制校准值
    for (uint8_t i = 0; i < 9; i++) {
        calibration_values[i] = calData->calibration_values[i];
    }
    
    return 1;
}

/**
 * @brief 检查Flash中的校准数据是否有效
 * @return 1=有效，0=无效
 */
uint8_t St1xFlash_IsCalibrationDataValid(void) {
    CalibrationData* calData = (CalibrationData*)CALIBRATION_FLASH_ADDRESS;
    
    // 检查魔术数字
    if (calData->magic_number != CALIBRATION_MAGIC_NUMBER) {
        return 0;
    }
    
    // 计算并验证校验和
    uint32_t calculatedChecksum = calculateChecksum((uint8_t*)calData->calibration_values, 
                                                   sizeof(calData->calibration_values));
    
    return (calculatedChecksum == calData->checksum) ? 1 : 0;
}

/**
 * @brief 擦除校准数据
 */
void St1xFlash_EraseCalibrationData(void) {
    // 解锁Flash
    if (unlockFlash() != HAL_OK) {
        return;
    }
    
    // 擦除目标页
    eraseFlashPage(CALIBRATION_FLASH_ADDRESS);
    
    // 锁定Flash
    lockFlash();
}