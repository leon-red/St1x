#ifndef __SH1107_ADVANCED_H
#define __SH1107_ADVANCED_H

#include "oled_driver.h"

// ==================== 高级图形绘制功能 ====================
void oled_draw_triangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t mode);
void oled_draw_filled_triangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t mode);
void oled_draw_rounded_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t mode);
void oled_draw_filled_rounded_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t mode);
void oled_draw_arc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t end_angle, uint8_t mode);

// ==================== 进度条和仪表功能 ====================
void oled_draw_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t mode);
void oled_draw_horizontal_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t mode);
void oled_draw_vertical_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t mode);
void oled_draw_gauge(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t end_angle, uint8_t value, uint8_t max_value, uint8_t mode);

// ==================== 高级文本功能 ====================
void oled_show_string_centered(uint8_t y, uint8_t *str, uint8_t size);
void oled_show_string_right_aligned(uint8_t x, uint8_t y, uint8_t *str, uint8_t size);
void oled_show_number_with_unit(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, const char* unit);

// ==================== 动画和特效功能 ====================
void oled_scroll_text_horizontal(uint8_t y, uint8_t *str, uint8_t size, uint8_t mode, uint16_t speed_ms);
void oled_fade_in(uint16_t duration_ms);
void oled_fade_out(uint16_t duration_ms);
void oled_wave_effect(uint16_t duration_ms);

// ==================== 屏幕管理功能 ====================
void oled_save_screen_state(void);
void oled_restore_screen_state(void);
void oled_create_snapshot(uint8_t snapshot_id);
void oled_restore_snapshot(uint8_t snapshot_id);

// ==================== 性能优化功能 ====================
void oled_enable_fast_mode(void);
void oled_disable_fast_mode(void);
void oled_set_refresh_rate(uint8_t rate_hz);

// ==================== 亮度控制基础功能 ====================
void oled_set_brightness(uint8_t brightness);
uint8_t oled_get_brightness(void);
void oled_set_brightness_level(uint8_t level);
uint8_t oled_get_brightness_level(void);

// ==================== 屏幕旋转功能（90度倍数） ====================
void oled_set_rotation(rotation_angle_t rotation);
rotation_angle_t oled_get_rotation(void);

// ==================== 调试和诊断功能 ====================
void oled_draw_test_pattern(void);
void oled_show_system_info(uint8_t x, uint8_t y);
void oled_show_memory_usage(uint8_t x, uint8_t y);

// ==================== 全局变量和数据结构 ====================
// 这些变量和数据结构由高级功能使用，基础驱动不依赖这些
extern const int16_t sin_table[360];
extern const int16_t cos_table[360];
extern uint8_t display_buffer[80][16];
extern uint8_t back_buffer[80][16];
extern rotation_angle_t current_rotation;
extern uint8_t current_brightness;

// ==================== 自动亮度控制功能 ====================
void oled_auto_brightness_increase(uint8_t step, uint16_t delay_ms);
void oled_auto_brightness_decrease(uint8_t step, uint16_t delay_ms);
void oled_auto_brightness_pulse(uint8_t min_brightness, uint8_t max_brightness, uint8_t step, uint16_t delay_ms, uint8_t cycles);
void oled_auto_brightness_breathing(uint8_t min_brightness, uint8_t max_brightness, uint16_t cycle_time_ms);

// ==================== 任意角度旋转支持 ====================
uint16_t oled_rotate_x_any(uint16_t angle, uint16_t x, uint16_t y, uint16_t center_x, uint16_t center_y);
uint16_t oled_rotate_y_any(uint16_t angle, uint16_t x, uint16_t y, uint16_t center_x, uint16_t center_y);
void oled_draw_line_any(uint16_t angle, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t mode);

#endif