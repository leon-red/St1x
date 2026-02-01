#ifndef __OLED_DRIVER_DMA_H
#define __OLED_DRIVER_DMA_H

#include "main.h"
#include "spi.h"
#include "gpio.h"

// 旋转角度类型定义（支持0~360度任意角度）
typedef enum {
    R0 = 0,    // 0度旋转
    R1 = 180,  // 180度旋转
    R2 = 90,   // 90度旋转
    R3 = 270   // 270度旋转
} rotation_angle_t;

//-----------------OLED端口定义---------------- 
#define OLED_Res_L HAL_GPIO_WritePin(OLED_Reset_GPIO_Port,OLED_Reset_Pin,GPIO_PIN_RESET)//RES
#define OLED_Res_H HAL_GPIO_WritePin(OLED_Reset_GPIO_Port,OLED_Reset_Pin,GPIO_PIN_SET)

#define OLED_DC_L  HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,GPIO_PIN_RESET)//DC
#define OLED_DC_H  HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,GPIO_PIN_SET)
	 
#define OLED_CS_L  HAL_GPIO_WritePin(OLED_CS_GPIO_Port,OLED_CS_Pin,GPIO_PIN_RESET)//CS
#define OLED_CS_H  HAL_GPIO_WritePin(OLED_CS_GPIO_Port,OLED_CS_Pin,GPIO_PIN_SET)

#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

// DMA传输状态
typedef enum {
    DMA_IDLE = 0,
    DMA_BUSY,
    DMA_COMPLETE,
    DMA_ERROR
} dma_status_t;

// 全局变量声明（使用原版驱动的全局变量）
extern uint8_t display_buffer[80][16];  // 80列×16页 = 80×128像素
extern uint8_t back_buffer[80][16];     // 后台缓冲区
extern rotation_angle_t current_rotation;
extern uint8_t current_brightness;

// DMA版本特有的全局变量
extern volatile dma_status_t dma_transfer_status;

// ==================== DMA版本驱动函数声明 ====================

// DMA传输控制
void oled_wait_complete(void);
void oled_transfer_data(uint8_t *data, uint16_t size, uint8_t cmd_type);

// 底层硬件驱动（DMA版本）
void oled_wr_bytes(uint8_t *data, uint16_t size, uint8_t cmd_type);
void oled_wr_cmds(uint8_t *cmds, uint16_t count);
void oled_wr_mixed(uint8_t *data, uint8_t *cmds, uint16_t count);

// 坐标转换和旋转控制
uint8_t oled_rotate_x(rotation_angle_t angle, uint8_t x, uint8_t y);
uint8_t oled_rotate_y(rotation_angle_t angle, uint8_t x, uint8_t y);
void oled_display_turn(rotation_angle_t angle);

// 基本显示控制（DMA优化版本）
void oled_display_on(void);
void oled_display_off(void);
void oled_color_turn(uint8_t i);
void oled_refresh(void);  // DMA优化的刷新函数
void oled_refresh_optimized(void);  // 优化的刷新函数（跳过复制步骤）
void oled_refresh_fullscreen(void); // 全屏刷新函数
void oled_clear(void);

// 像素级操作
void oled_draw_point(uint8_t x, uint8_t y, uint8_t t);
void oled_clear_point(uint8_t x, uint8_t y);

// 基础图形绘制
void oled_draw_hline(uint8_t x, uint8_t y, uint8_t len, uint8_t mode);
void oled_draw_vline(uint8_t x, uint8_t y, uint8_t len, uint8_t mode);
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);
void oled_draw_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);
void oled_draw_filled_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);
void oled_draw_circle(uint8_t x, uint8_t y, uint8_t r, uint8_t mode);
void oled_draw_filled_circle(uint8_t x, uint8_t y, uint8_t r, uint8_t mode);

// 字符和文本显示
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode);
void oled_show_string(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1);
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode);
void oled_show_chinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1, uint8_t mode);
void oled_printf(uint8_t x, uint8_t y, uint8_t size, uint8_t mode, const char *format, ...);

// 高级功能和控制
void oled_init(void);
void oled_init_optimized(void);  // 优化的初始化函数
void oled_clear_area(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void oled_refresh_area(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void oled_set_display(uint8_t on);

// 性能测试函数
void oled_performance_test(void);

#endif