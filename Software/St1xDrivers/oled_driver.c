#include "oled_driver.h"
#include "stdlib.h"
#include "oledfont.h"
#include "spi.h"
#include <stdarg.h>
#include <stdio.h>

// 使用原版驱动的全局变量（extern声明在头文件中）

// DMA版本特有的全局变量
volatile dma_status_t dma_transfer_status = DMA_IDLE;

// DMA传输缓冲区（用于批量数据传输）
static uint8_t dma_buffer[1280];  // 80*16 = 1280字节，足够存储整个显示缓冲区

// ==================== DMA传输控制函数 ====================

/**
 * @brief DMA传输完成回调函数
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi->Instance == SPI2) {
        dma_transfer_status = DMA_COMPLETE;
    }
}

/**
 * @brief DMA传输错误回调函数
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi->Instance == SPI2) {
        dma_transfer_status = DMA_ERROR;
    }
}

/**
 * @brief 等待DMA传输完成
 */
void oled_wait_complete(void)
{
    while(dma_transfer_status == DMA_BUSY) {
        // 等待传输完成
    }
    
    if(dma_transfer_status == DMA_ERROR) {
        // DMA传输错误处理
        dma_transfer_status = DMA_IDLE;
    } else {
        dma_transfer_status = DMA_IDLE;
    }
}

/**
 * @brief DMA传输数据
 * @param data 数据指针
 * @param size 数据大小
 * @param cmd_type 命令类型（OLED_CMD或OLED_DATA）
 */
void oled_transfer_data(uint8_t *data, uint16_t size, uint8_t cmd_type)
{
    if(dma_transfer_status == DMA_BUSY) {
        oled_wait_complete();
    }
    
    OLED_CS_L;
    
    if(cmd_type == OLED_CMD) {
        OLED_DC_L;
    } else {
        OLED_DC_H;
    }
    
    dma_transfer_status = DMA_BUSY;
    
    if(HAL_SPI_Transmit_DMA(&hspi2, data, size) != HAL_OK) {
        dma_transfer_status = DMA_ERROR;
    }
    
    oled_wait_complete();
    
    OLED_CS_H;
    OLED_DC_H;
}

// ==================== 第1层：最底层硬件驱动（DMA版本） ====================

/**
 * @brief DMA版本的批量数据写入（推荐使用）
 */
void oled_wr_bytes(uint8_t *data, uint16_t size, uint8_t cmd_type)
{
    oled_transfer_data(data, size, cmd_type);
}

/**
 * @brief DMA版本的批量命令发送（专用函数）
 * @param cmds 命令数组
 * @param count 命令数量
 */
void oled_wr_cmds(uint8_t *cmds, uint16_t count)
{
    oled_transfer_data(cmds, count, OLED_CMD);
}

/**
 * @brief DMA版本的混合数据发送（命令+数据）
 * @param data 数据数组
 * @param cmds 命令数组（可以为NULL）
 * @param count 数据/命令数量
 */
void oled_wr_mixed(uint8_t *data, uint8_t *cmds, uint16_t count)
{
    if(cmds != NULL) {
        // 发送命令
        oled_transfer_data(cmds, count, OLED_CMD);
    }
    // 发送数据
    oled_transfer_data(data, count, OLED_DATA);
}

// ==================== 第2层：坐标转换和旋转控制 ====================

uint8_t oled_rotate_x(rotation_angle_t angle, uint8_t x, uint8_t y)
{
    switch(angle) {
        case R0:
            return x;
        case R1:
            return 79 - y;
        case R2:
            return 79 - x;
        case R3:
            return y;
        default:
            return x;
    }
}

uint8_t oled_rotate_y(rotation_angle_t angle, uint8_t x, uint8_t y)
{
    switch(angle) {
        case R0:
            return y;
        case R1:
            return x;
        case R2:
            return 127 - y;
        case R3:
            return 127 - x;
        default:
            return y;
    }
}

void oled_display_turn(rotation_angle_t angle)
{
    current_rotation = angle;
    
    uint8_t cmd_buffer[4] = {0xA0, 0xC0, 0xD3, 0x68};
    oled_transfer_data(cmd_buffer, 4, OLED_CMD);
}

// ==================== 第3层：基本显示控制（DMA优化版本） ====================

void oled_display_on(void)
{
    uint8_t cmd_buffer[3] = {0x8D, 0x14, 0xAF};
    oled_transfer_data(cmd_buffer, 3, OLED_CMD);
}

void oled_display_off(void)
{
    uint8_t cmd_buffer[1] = {0xAE};
    oled_wr_cmds(cmd_buffer, sizeof(cmd_buffer));
}

void oled_color_turn(uint8_t i)
{
    uint8_t cmd_buffer[1] = {i ? 0xA7 : 0xA6};
    oled_wr_cmds(cmd_buffer, sizeof(cmd_buffer));
}

/**
 * @brief DMA优化的刷新函数 - 使用批量传输提高效率
 */
void oled_refresh(void)
{
    // 双缓冲机制：将后台缓冲区复制到前台缓冲区
    for(uint8_t i = 0; i < 16; i++) {
        for(uint8_t n = 0; n < 80; n++) {
            display_buffer[n][i] = back_buffer[n][i];
        }
    }
    
    // DMA优化：批量传输整个显示缓冲区
    uint16_t buffer_index = 0;
    
    for(uint8_t i = 0; i < 16; i++) {
        // 设置页地址命令
        uint8_t page_cmd[3] = {0xB0 + i, 0x00, 0x10};
        oled_transfer_data(page_cmd, 3, OLED_CMD);
        
        // 批量传输该页的所有列数据
        for(uint8_t n = 0; n < 80; n++) {
            dma_buffer[buffer_index++] = display_buffer[n][i];
        }
        
        // 传输该页的数据
        oled_transfer_data(&dma_buffer[buffer_index - 80], 80, OLED_DATA);
        buffer_index = 0;  // 重置缓冲区索引
    }
}

/**
 * @brief DMA优化的刷新函数 - 直接使用后台缓冲区，跳过复制步骤
 */
void oled_refresh_optimized(void)
{
    // 直接使用后台缓冲区进行刷新，跳过复制步骤
    uint16_t buffer_index = 0;
    
    for(uint8_t i = 0; i < 16; i++) {
        // 设置页地址命令
        uint8_t page_cmd[3] = {0xB0 + i, 0x00, 0x10};
        oled_transfer_data(page_cmd, 3, OLED_CMD);
        
        // 批量传输该页的所有列数据（直接从后台缓冲区）
        for(uint8_t n = 0; n < 80; n++) {
            dma_buffer[buffer_index++] = back_buffer[n][i];
        }
        
        // 传输该页的数据
        oled_transfer_data(&dma_buffer[buffer_index - 80], 80, OLED_DATA);
        buffer_index = 0;  // 重置缓冲区索引
    }
}

/**
 * @brief DMA优化的全屏刷新函数 - 一次性发送所有数据
 */
void oled_refresh_fullscreen(void)
{
    // 准备全屏刷新命令和数据
    uint8_t fullscreen_cmds[16 * 3];  // 16页，每页3个命令
    uint16_t data_index = 0;
    
    // 准备所有页的命令
    for(uint8_t i = 0; i < 16; i++) {
        fullscreen_cmds[i * 3] = 0xB0 + i;    // 页地址
        fullscreen_cmds[i * 3 + 1] = 0x00;    // 列地址低字节
        fullscreen_cmds[i * 3 + 2] = 0x10;    // 列地址高字节
    }
    
    // 准备所有页的数据
    for(uint8_t i = 0; i < 16; i++) {
        for(uint8_t n = 0; n < 80; n++) {
            dma_buffer[data_index++] = back_buffer[n][i];
        }
    }
    
    // 一次性发送所有命令和数据
    for(uint8_t i = 0; i < 16; i++) {
        // 发送页设置命令
        oled_transfer_data(&fullscreen_cmds[i * 3], 3, OLED_CMD);
        // 发送该页的数据
        oled_transfer_data(&dma_buffer[i * 80], 80, OLED_DATA);
    }
}

void oled_clear(void)
{
    // 清空后台缓冲区
    for(uint8_t i = 0; i < 16; i++) {
        for(uint8_t n = 0; n < 80; n++) {
            back_buffer[n][i] = 0;
        }
    }
    // 刷新显示
    oled_refresh();
}

// ==================== 第4层：像素级操作 ====================

void oled_draw_point(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t rotated_x = oled_rotate_x(current_rotation, x, y);
    uint8_t rotated_y = oled_rotate_y(current_rotation, x, y);

    uint8_t page = rotated_y / 8;
    uint8_t bit_mask = 1 << (rotated_y % 8);

    if(t) {
        back_buffer[rotated_x][page] |= bit_mask;
    } else {
        back_buffer[rotated_x][page] &= ~bit_mask;
    }
}

void oled_clear_point(uint8_t x, uint8_t y)
{
    oled_draw_point(x, y, 0);
}

// ==================== 第5层：基础图形绘制 ====================

void oled_draw_hline(uint8_t x, uint8_t y, uint8_t len, uint8_t mode)
{
    if(y >= 128) return;
    if(x >= 80) return;
    if(x + len > 80) len = 80 - x;

    for(uint8_t i = 0; i < len; i++) {
        oled_draw_point(x + i, y, mode);
    }
}

void oled_draw_vline(uint8_t x, uint8_t y, uint8_t len, uint8_t mode)
{
    if(x >= 80) return;
    if(y >= 128) return;
    if(y + len > 128) len = 128 - y;

    for(uint8_t i = 0; i < len; i++) {
        oled_draw_point(x, y + i, mode);
    }
}

void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode)
{
    int16_t dx = abs((int16_t)x2 - (int16_t)x1);
    int16_t dy = abs((int16_t)y2 - (int16_t)y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;

    while(1) {
        oled_draw_point(x1, y1, mode);

        if(x1 == x2 && y1 == y2) break;

        int16_t e2 = 2 * err;
        if(e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if(e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void oled_draw_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode)
{
    oled_draw_line(x1, y1, x2, y1, mode);  // 上边
    oled_draw_line(x1, y2, x2, y2, mode);  // 下边
    oled_draw_line(x1, y1, x1, y2, mode);  // 左边
    oled_draw_line(x2, y1, x2, y2, mode);  // 右边
}

void oled_draw_filled_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode)
{
    if(x1 > x2) { uint8_t temp = x1; x1 = x2; x2 = temp; }
    if(y1 > y2) { uint8_t temp = y1; y1 = y2; y2 = temp; }

    for(uint8_t y = y1; y <= y2; y++) {
        for(uint8_t x = x1; x <= x2; x++) {
            oled_draw_point(x, y, mode);
        }
    }
}

void oled_draw_circle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t mode)
{
    uint8_t max_x, max_y;
    switch(current_rotation) {
        case R0:
        case R2:
            max_x = 79;
            max_y = 127;
            break;
        case R1:
        case R3:
            max_x = 127;
            max_y = 79;
            break;
    }
    
    if(x0 >= max_x || y0 >= max_y) return;
    if(r > max_x || r > max_y) {
        r = (max_x < max_y) ? max_x : max_y;
    }
    
    int16_t x = 0;
    int16_t y = r;
    int16_t d = 3 - 2 * r;

    while(x <= y) {
        oled_draw_point(x0 + x, y0 + y, mode);
        oled_draw_point(x0 - x, y0 + y, mode);
        oled_draw_point(x0 + x, y0 - y, mode);
        oled_draw_point(x0 - x, y0 - y, mode);
        oled_draw_point(x0 + y, y0 + x, mode);
        oled_draw_point(x0 - y, y0 + x, mode);
        oled_draw_point(x0 + y, y0 - x, mode);
        oled_draw_point(x0 - y, y0 - x, mode);

        if(d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void oled_draw_filled_circle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t mode)
{
    uint8_t max_x, max_y;
    switch(current_rotation) {
        case R0:
        case R2:
            max_x = 79;
            max_y = 127;
            break;
        case R1:
        case R3:
            max_x = 127;
            max_y = 79;
            break;
    }
    
    if(x0 >= max_x || y0 >= max_y) return;
    if(r > max_x || r > max_y) {
        r = (max_x < max_y) ? max_x : max_y;
    }
    
    int16_t x = 0;
    int16_t y = r;
    int16_t d = 3 - 2 * r;

    while(x <= y) {
        oled_draw_line(x0 - x, y0 + y, x0 + x, y0 + y, mode);
        oled_draw_line(x0 - x, y0 - y, x0 + x, y0 - y, mode);
        oled_draw_line(x0 - y, y0 + x, x0 + y, y0 + x, mode);
        oled_draw_line(x0 - y, y0 - x, x0 + y, y0 - x, mode);

        if(d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

// ==================== 第6层：字符和文本显示 ====================

void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode)
{
    uint8_t char_width, char_height;
    uint8_t char_index = chr - ' ';
    uint8_t x0 = x, y0 = y;

    switch(size1) {
        case 8: char_width = 6; char_height = 8; break;
        case 12: char_width = 6; char_height = 12; break;
        case 16: char_width = 8; char_height = 16; break;
        case 24: char_width = 12; char_height = 24; break;
        default: return;
    }

    uint8_t max_x, max_y;
    switch(current_rotation) {
        case R0:
        case R2:
            max_x = 79;
            max_y = 127;
            break;
        case R1:
        case R3:
            max_x = 127;
            max_y = 79;
            break;
    }

    if (x >= max_x || y >= max_y ||
        x + char_width > max_x || y + char_height > max_y) {
        return;
    }

    uint8_t data_size;
    if(size1 == 8) {
        data_size = 6;
    } else {
        data_size = (size1/8 + ((size1%8)?1:0)) * (size1/2);
    }

    for(uint8_t i = 0; i < data_size; i++) {
        uint8_t temp;

        switch(size1) {
            case 8:
                temp = asc2_0806[char_index][i];
                break;
            case 12:
                temp = asc2_1206[char_index][i];
                break;
            case 16:
                temp = asc2_1608[char_index][i];
                break;
            case 24:
                temp = asc2_2412[char_index][i];
                break;
            default:
                return;
        }

        for(uint8_t bit = 0; bit < 8; bit++) {
            if(temp & 0x01) {
                oled_draw_point(x, y, mode);
            } else {
                oled_draw_point(x, y, !mode);
            }
            temp >>= 1;
            y++;
        }
        x++;

        if(size1 != 8) {
            if((x - x0) == size1/2) {
                x = x0;
                y0 = y0 + 8;
            }
        }
        y = y0;
    }
}

void oled_show_string(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1)
{
    while((*chr >= ' ') && (*chr <= '~')) {
        oled_show_char(x, y, *chr, size1, 1);

        if(size1 == 8) x += 6;
        else x += size1 / 2;

        chr++;
    }
}

uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while(n--) result *= m;
    return result;
}

void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode)
{
    uint8_t m = (size1 == 8) ? 2 : 0;
    uint8_t char_spacing = size1 / 2 + m;

    for(uint8_t t = 0; t < len; t++) {
        uint8_t temp = (num / oled_pow(10, len - t - 1)) % 10;
        oled_show_char(x, y, temp + '0', size1, mode);

        x += char_spacing;
    }
}

void oled_show_chinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1, uint8_t mode)
{
    uint8_t bytes_per_col = (size1 + 7) / 8;
    uint8_t x0 = x, y0 = y;

    for(uint8_t i = 0; i < size1; i++) {
        uint8_t font_data[8] = {0};

        switch(size1) {
            case 16:
                for(uint8_t j = 0; j < bytes_per_col; j++) font_data[j] = Hzk1[num][i * bytes_per_col + j];
                break;
            case 24:
                for(uint8_t j = 0; j < bytes_per_col; j++) font_data[j] = Hzk2[num][i * bytes_per_col + j];
                break;
            case 32:
                for(uint8_t j = 0; j < bytes_per_col; j++) font_data[j] = Hzk3[num][i * bytes_per_col + j];
                break;
            case 64:
                for(uint8_t j = 0; j < bytes_per_col; j++) font_data[j] = Hzk4[num][i * bytes_per_col + j];
                break;
            default: return;
        }

        for(uint8_t j = 0; j < bytes_per_col; j++) {
            uint8_t temp = font_data[j];
            for(uint8_t bit = 0; bit < 8 && (j * 8 + bit) < size1; bit++) {
                oled_draw_point(x, y + j * 8 + bit, (temp & (1 << bit)) ? mode : !mode);
            }
        }
        x++;
        if((x - x0) == size1) { x = x0; y0 += size1; }
        y = y0;
    }
}

void oled_printf(uint8_t x, uint8_t y, uint8_t size, uint8_t mode, const char *format, ...)
{
    char buffer[265];
    char *p = buffer;
    uint8_t line_height = size;

    // Fix character width calculation: set character width correctly based on font size
    uint8_t char_width;
    switch(size) {
        case 8: char_width = 6; break;
        case 12: char_width = 6; break;
        case 16: char_width = 8; break;
        case 24: char_width = 12; break;
        default: char_width = 6; break;
    }

    uint8_t current_x = x;
    uint8_t current_y = y;

    // Fix screen size adaptation: set max width and height correctly based on rotation angle
    uint8_t max_width, max_height;
    switch(current_rotation) {
        case R0:
        case R2:
            max_width = 80;
            max_height = 128;
            break;
        case R1:
        case R3:
            max_width = 128;
            max_height = 80;
            break;
        default:
            max_width = 80;
            max_height = 128;
            break;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    while (*p != '\0') {
        // Fix newline logic: simplify processing logic, ensure correct position after newline
        if (*p == '\n') {
            current_x = x;
            current_y += line_height;
            p++;
            continue;
        }
        
        // Fix carriage return logic
        if (*p == '\r') {
            current_x = x;
            p++;
            continue;
        }
        
        // Fix tab character logic
        if (*p == '\t') {
            uint8_t tab_width = char_width * 4;
            if (current_x + tab_width < max_width) {
                current_x += tab_width;
            } else {
                current_x = x;
                current_y += line_height;
            }
            p++;
            continue;
        }
        
        // Fix boundary check: enhance boundary checking logic
        if (current_x + char_width > max_width) {
            current_x = x;
            current_y += line_height;
        }
        
        // Fix vertical boundary check
        if (current_y + size > max_height) {
            break;
        }
        
        // Fix character display: ensure characters are displayed within valid range
        if (current_x < max_width && current_y < max_height) {
            oled_show_char(current_x, current_y, *p, size, mode);
            current_x += char_width;
        }
        
        p++;
    }
}

// ==================== 第7层：高级功能和控制 ====================

void oled_init(void)
{
    OLED_Res_L;
    HAL_Delay(200);
    OLED_Res_H;

    // 使用批量命令传输优化初始化过程
    uint8_t init_cmds[] = {
        0xAE, 0x00, 0x10, 0x20, 0x81, current_brightness, 0xA0, 0xC0,
        0xA4, 0xA6, 0xD5, 0x91, 0xD9, 0x22, 0xdb, 0x3f,
        0xA8, 0x4F, 0xD3, 0x68, 0xdc, 0x00, 0xad, 0x8a
    };
    
    oled_transfer_data(init_cmds, sizeof(init_cmds), OLED_CMD);
    oled_clear();
    
    // 使用批量命令开启显示
    uint8_t display_on_cmd = 0xAF;
    oled_transfer_data(&display_on_cmd, 1, OLED_CMD);
}

/**
 * @brief DMA优化的初始化函数 - 使用专用批量命令发送函数
 */
void oled_init_optimized(void)
{
    OLED_Res_L;
    HAL_Delay(200);
    OLED_Res_H;

    // 使用优化的初始化命令序列
    uint8_t init_cmds[] = {
        0xAE,       // 关闭显示
        0x00, 0x10, // 设置列地址
        0x20,       // 设置内存地址模式
        0x81, current_brightness, // 设置对比度
        0xA0, 0xC0, // 设置段重映射和COM扫描方向
        0xA4,       // 关闭全局显示
        0xA6,       // 设置正常显示
        0xD5, 0x91, // 设置显示时钟分频比/振荡器频率
        0xD9, 0x22, // 设置预充电周期
        0xdb, 0x3f, // 设置VCOMH电压
        0xA8, 0x4F, // 设置多路复用率
        0xD3, 0x68, // 设置显示偏移
        0xdc, 0x00, // 设置显示起始行
        0xad, 0x8a  // 设置DC-DC使能
    };
    
    // 使用专用批量命令发送函数
    oled_wr_cmds(init_cmds, sizeof(init_cmds));
    oled_clear();
    
    // 开启显示
    uint8_t display_on_cmd = 0xAF;
    oled_wr_cmds(&display_on_cmd, 1);
}

void oled_clear_area(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t max_x = (current_rotation == R1 || current_rotation == R3) ? 127 : 79;
    uint8_t max_y = (current_rotation == R1 || current_rotation == R3) ? 79 : 127;

    if (x > max_x || y > max_y) return;

    if (x + width > max_x + 1) width = max_x + 1 - x;
    if (y + height > max_y + 1) height = max_y + 1 - y;

    for (uint8_t i = 0; i < width; i++) {
        for (uint8_t j = 0; j < height; j++) {
            oled_clear_point(x + i, y + j);
        }
    }
}

void oled_refresh_area(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t max_x = (current_rotation == R1 || current_rotation == R3) ? 127 : 79;
    uint8_t max_y = (current_rotation == R1 || current_rotation == R3) ? 79 : 127;

    if (x > max_x || y > max_y) return;

    if (x + width > max_x + 1) width = max_x + 1 - x;
    if (y + height > max_y + 1) height = max_y + 1 - y;

    if (current_rotation != R0) {
        oled_refresh();
        return;
    }

    uint8_t start_page = y / 8;
    uint8_t end_page = (y + height - 1) / 8;

    for (uint8_t page = start_page; page <= end_page; page++) {
        uint8_t page_cmd[3] = {0xB0 + page, 0x00, 0x10};
        oled_transfer_data(page_cmd, 3, OLED_CMD);

        uint8_t start_col = x;
        uint8_t end_col = x + width - 1;

        // 批量传输该区域的数据
        uint16_t buffer_index = 0;
        for (uint8_t col = start_col; col <= end_col; col++) {
            dma_buffer[buffer_index++] = display_buffer[col][page];
        }
        
        oled_transfer_data(dma_buffer, buffer_index, OLED_DATA);
    }
}

void oled_set_display(uint8_t on)
{
    if (on) {
        oled_display_on();
    } else {
        oled_display_off();
    }
}