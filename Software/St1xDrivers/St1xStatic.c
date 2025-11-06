#include "St1xStatic.h"
#include "lis2dw12_reg.h"
#include "spi.h"
#include <stdio.h>
#include <string.h>

// 传感器相关变量
static stmdev_ctx_t dev_ctx;
static int16_t data_raw_acceleration[3];
static float acceleration_mg[3];
static uint8_t whoamI;
static uint8_t sensor_initialized = 0;

// SPI读写函数声明
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void platform_delay(uint32_t ms);

/**
 * @brief 初始化静态传感器显示模块
 */
void St1xStatic_Init(void) {
    if (sensor_initialized) return;
    
    // 初始化传感器驱动接口
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.mdelay = platform_delay;
    dev_ctx.handle = &hspi2;  // 使用SPI2接口
    
    // 等待传感器启动
    platform_delay(20);
    
    // 检查设备ID
    lis2dw12_device_id_get(&dev_ctx, &whoamI);
    
    if (whoamI == LIS2DW12_ID) {
        uint8_t rst;
        // 恢复默认配置
        lis2dw12_reset_set(&dev_ctx, PROPERTY_ENABLE);
        
        do {
            lis2dw12_reset_get(&dev_ctx, &rst);
        } while (rst);
        
        // 启用块数据更新
        lis2dw12_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
        
        // 设置满量程
        lis2dw12_full_scale_set(&dev_ctx, LIS2DW12_2g);
        
        // 配置滤波链
        lis2dw12_filter_path_set(&dev_ctx, LIS2DW12_LPF_ON_OUT);
        lis2dw12_filter_bandwidth_set(&dev_ctx, LIS2DW12_ODR_DIV_4);
        
        // 配置功耗模式
        lis2dw12_power_mode_set(&dev_ctx, LIS2DW12_HIGH_PERFORMANCE);
        
        // 设置输出数据速率
        lis2dw12_data_rate_set(&dev_ctx, LIS2DW12_XL_ODR_25Hz);
        
        sensor_initialized = 1;
    }
}

/**
 * @brief 显示6轴传感器数据
 * @param u8g2 u8g2显示对象指针
 */
void St1xStatic_Display(u8g2_t* u8g2) {
    uint8_t reg;
    
    // 读取数据前先检查传感器是否已初始化
    if (!sensor_initialized) {
        u8g2_ClearBuffer(u8g2);
        u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(u8g2, 0, 20, "Sensor Not Found!");
        u8g2_SendBuffer(u8g2);
        return;
    }
    
    // 检查是否有新数据
    lis2dw12_flag_data_ready_get(&dev_ctx, &reg);
    
    if (reg) {
        // 读取加速度数据
        memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
        lis2dw12_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
        
        // 转换为mg单位
        acceleration_mg[0] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[0]);
        acceleration_mg[1] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[1]);
        acceleration_mg[2] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[2]);
    }
    
    // 显示数据
    u8g2_ClearBuffer(u8g2);
    
    // 显示标题
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 0, 10, "6-Axis Sensor Data");
    
    // 显示X轴数据
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    char x_str[20];
    sprintf(x_str, "X: %6.2f mg", acceleration_mg[0]);
    u8g2_DrawStr(u8g2, 0, 25, x_str);
    
    // 显示Y轴数据
    char y_str[20];
    sprintf(y_str, "Y: %6.2f mg", acceleration_mg[1]);
    u8g2_DrawStr(u8g2, 0, 37, y_str);
    
    // 显示Z轴数据
    char z_str[20];
    sprintf(z_str, "Z: %6.2f mg", acceleration_mg[2]);
    u8g2_DrawStr(u8g2, 0, 49, z_str);
    
    // 显示设备ID
    char id_str[20];
    sprintf(id_str, "ID: 0x%02X", whoamI);
    u8g2_DrawStr(u8g2, 0, 61, id_str);
    
    u8g2_SendBuffer(u8g2);
}

/**
 * @brief 菜单项动作函数
 */
void St1xStatic_Action(void) {
    // 初始化传感器
    St1xStatic_Init();
}

/**
 * @brief 传感器数据显示函数
 */
void St1xStatic_DisplayData(u8g2_t* u8g2) {
    uint8_t reg;
    
    // 读取数据前先检查传感器是否已初始化
    if (!sensor_initialized) {
        u8g2_ClearBuffer(u8g2);
        u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(u8g2, 0, 20, "Sensor Not Found!");
        u8g2_SendBuffer(u8g2);
        return;
    }
    
    // 检查是否有新数据
    lis2dw12_flag_data_ready_get(&dev_ctx, &reg);
    
    if (reg) {
        // 读取加速度数据
        memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
        lis2dw12_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
        
        // 转换为mg单位
        acceleration_mg[0] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[0]);
        acceleration_mg[1] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[1]);
        acceleration_mg[2] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[2]);
    }
    
    // 显示数据
    u8g2_ClearBuffer(u8g2);
    
    // 显示标题
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 0, 10, "6-Axis Sensor Data");
    
    // 显示X轴数据
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    char x_str[20];
    sprintf(x_str, "X: %6.2f mg", acceleration_mg[0]);
    u8g2_DrawStr(u8g2, 0, 25, x_str);
    
    // 显示Y轴数据
    char y_str[20];
    sprintf(y_str, "Y: %6.2f mg", acceleration_mg[1]);
    u8g2_DrawStr(u8g2, 0, 37, y_str);
    
    // 显示Z轴数据
    char z_str[20];
    sprintf(z_str, "Z: %6.2f mg", acceleration_mg[2]);
    u8g2_DrawStr(u8g2, 0, 49, z_str);
    
    // 显示设备ID
    char id_str[20];
    sprintf(id_str, "ID: 0x%02X", whoamI);
    u8g2_DrawStr(u8g2, 0, 61, id_str);
    
    u8g2_SendBuffer(u8g2);
}

/*
 * @brief 写入设备寄存器 (平台相关)
 * @param handle 设备句柄
 * @param reg 寄存器地址
 * @param bufp 要写入的数据缓冲区
 * @param len 数据长度
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &reg, 1, 1000);
    HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_SET);
    return 0;
}

/*
 * @brief 读取设备寄存器 (平台相关)
 * @param handle 设备句柄
 * @param reg 寄存器地址
 * @param bufp 读取数据缓冲区
 * @param len 数据长度
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    reg |= 0x80;
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &reg, 1, 1000);
    HAL_SPI_Receive(handle, bufp, len, 1000);
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_SET);
    return 0;
}

/*
 * @brief 平台延迟函数
 * @param ms 延迟毫秒数
 */
static void platform_delay(uint32_t ms) {
    HAL_Delay(ms);
}