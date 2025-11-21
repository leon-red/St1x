#include "oled_advanced.h"
#include "stdlib.h"
#include "oledfont.h"
#include "spi.h"
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

// ==================== 全局变量和数据结构 ====================
// 这些变量和数据结构由高级功能使用，基础驱动不依赖这些

// sin/cos查找表定义（360度，缩放因子1000）
const int16_t sin_table[360] = {
    0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 174, 191, 208, 225, 242, 259, 276, 292, 309, 325,
    342, 358, 374, 390, 406, 422, 438, 454, 469, 485, 500, 515, 530, 545, 560, 574, 589, 603, 617, 631,
    645, 659, 672, 685, 698, 711, 724, 737, 749, 761, 773, 785, 797, 809, 820, 831, 842, 853, 864, 874,
    885, 895, 905, 914, 924, 933, 942, 951, 960, 969, 977, 985, 993, 1000, 1008, 1015, 1022, 1029, 1035, 1042,
    1048, 1054, 1060, 1065, 1071, 1076, 1081, 1086, 1090, 1095, 1099, 1103, 1106, 1110, 1113, 1116, 1119, 1121, 1124, 1126,
    1128, 1129, 1131, 1132, 1133, 1134, 1134, 1135, 1135, 1135, 1135, 1134, 1134, 1133, 1132, 1131, 1129, 1128, 1126, 1124,
    1121, 1119, 1116, 1113, 1110, 1106, 1103, 1099, 1095, 1090, 1086, 1081, 1076, 1071, 1065, 1060, 1054, 1048, 1042, 1035,
    1029, 1022, 1015, 1008, 1000, 993, 985, 977, 969, 960, 951, 942, 933, 924, 914, 905, 895, 885, 874, 864,
    853, 842, 831, 820, 809, 797, 785, 773, 761, 749, 737, 724, 711, 698, 685, 672, 659, 645, 631, 617,
    603, 589, 574, 560, 545, 530, 515, 500, 485, 469, 454, 438, 422, 406, 390, 374, 358, 342, 325, 309,
    292, 276, 259, 242, 225, 208, 191, 174, 156, 139, 122, 105, 87, 70, 52, 35, 17, 0, -17, -35,
    -52, -70, -87, -105, -122, -139, -156, -174, -191, -208, -225, -242, -259, -276, -292, -309, -325, -342, -358, -374,
    -390, -406, -422, -438, -454, -469, -485, -500, -515, -530, -545, -560, -574, -589, -603, -617, -631, -645, -659, -672,
    -685, -698, -711, -724, -737, -749, -761, -773, -785, -797, -809, -820, -831, -842, -853, -864, -874, -885, -895, -905,
    -914, -924, -933, -942, -951, -960, -969, -977, -985, -993, -1000, -1008, -1015, -1022, -1029, -1035, -1042, -1048, -1054, -1060,
    -1065, -1071, -1076, -1081, -1086, -1090, -1095, -1099, -1103, -1106, -1110, -1113, -1116, -1119, -1121, -1124, -1126, -1128, -1129, -1131,
    -1132, -1133, -1134, -1134, -1135, -1135, -1135, -1135, -1134, -1134, -1133, -1132, -1131, -1129, -1128, -1126, -1124, -1121, -1119, -1116,
    -1113, -1110, -1106, -1103, -1099, -1095, -1090, -1086, -1081, -1076, -1071, -1065, -1060, -1054, -1048, -1042, -1035, -1029, -1022, -1015
};

const int16_t cos_table[360] = {
    1000, 1000, 999, 998, 997, 996, 994, 992, 990, 988, 985, 982, 979, 976, 972, 968, 964, 960, 955, 951,
    946, 940, 935, 929, 924, 918, 912, 905, 899, 892, 885, 878, 871, 863, 856, 848, 840, 832, 823, 815,
    806, 797, 788, 779, 770, 760, 751, 741, 731, 721, 711, 700, 690, 679, 669, 658, 647, 636, 624, 613,
    601, 590, 578, 566, 554, 542, 530, 518, 506, 493, 481, 468, 456, 443, 430, 417, 404, 391, 378, 365,
    352, 339, 326, 312, 299, 286, 272, 259, 245, 232, 218, 205, 191, 178, 164, 151, 137, 124, 110, 97,
    83, 70, 56, 43, 29, 16, 2, -11, -25, -38, -52, -65, -79, -92, -105, -119, -132, -145, -158, -171,
    -184, -197, -210, -223, -236, -248, -261, -273, -286, -298, -310, -322, -334, -346, -358, -370, -381, -393, -404, -415,
    -426, -437, -448, -459, -469, -480, -490, -500, -510, -520, -530, -540, -549, -559, -568, -577, -586, -595, -604, -612,
    -621, -629, -637, -645, -653, -661, -668, -676, -683, -690, -697, -704, -710, -717, -723, -729, -735, -741, -747, -752,
    -757, -763, -768, -772, -777, -781, -786, -790, -794, -798, -801, -805, -808, -811, -814, -817, -819, -822, -824, -826,
    -828, -830, -831, -833, -834, -835, -836, -837, -837, -838, -838, -838, -838, -838, -837, -837, -836, -835, -834, -833,
    -831, -830, -828, -826, -824, -822, -819, -817, -814, -811, -808, -805, -801, -798, -794, -790, -786, -781, -777, -772,
    -768, -763, -757, -752, -747, -741, -735, -729, -723, -717, -710, -704, -697, -690, -683, -676, -668, -661, -653, -645,
    -637, -629, -621, -612, -604, -595, -586, -577, -568, -559, -549, -540, -530, -520, -510, -500, -490, -480, -469, -459,
    -448, -437, -426, -415, -404, -393, -381, -370, -358, -346, -334, -322, -310, -298, -286, -273, -261, -248, -236, -223,
    -210, -197, -184, -171, -158, -145, -132, -119, -105, -92, -79, -65, -52, -38, -25, -11, 2, 16, 29, 43,
    56, 70, 83, 97, 110, 124, 137, 151, 164, 178, 191, 205, 218, 232, 245, 259, 272, 286, 299, 312,
    326, 339, 352, 365, 378, 391, 404, 417, 430, 443, 456, 468, 481, 493, 506, 518, 530, 542, 554, 566,
};

uint8_t display_buffer[80][16];  // 前台缓冲区：80列×16页 = 80×128像素
uint8_t back_buffer[80][16];       // 后台缓冲区：80列×16页 = 80×128像素
rotation_angle_t current_rotation = R0;
uint8_t current_brightness = 0x81;  // 默认亮度值

// ==================== 高级图形绘制功能 ====================

// 绘制三角形
void oled_draw_triangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t mode)
{
    oled_draw_line(x1, y1, x2, y2, mode);
    oled_draw_line(x2, y2, x3, y3, mode);
    oled_draw_line(x3, y3, x1, y1, mode);
}

// 绘制填充三角形
void oled_draw_filled_triangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t mode)
{
    // 使用扫描线算法填充三角形
    int16_t temp;
    
    // 按y坐标排序顶点
    if (y1 > y2) { temp = y1; y1 = y2; y2 = temp; temp = x1; x1 = x2; x2 = temp; }
    if (y1 > y3) { temp = y1; y1 = y3; y3 = temp; temp = x1; x1 = x3; x3 = temp; }
    if (y2 > y3) { temp = y2; y2 = y3; y3 = temp; temp = x2; x2 = x3; x3 = temp; }
    
    int16_t dx1 = x2 - x1, dy1 = y2 - y1;
    int16_t dx2 = x3 - x1, dy2 = y3 - y1;
    
    // 绘制上半部分
    for (int16_t y = y1; y <= y2; y++) {
        int16_t ax = x1 + dx1 * (y - y1) / dy1;
        int16_t bx = x1 + dx2 * (y - y1) / dy2;
        
        if (ax > bx) { temp = ax; ax = bx; bx = temp; }
        
        for (int16_t x = ax; x <= bx; x++) {
            oled_draw_point(x, y, mode);
        }
    }
    
    // 绘制下半部分
    dx1 = x3 - x2; dy1 = y3 - y2;
    
    for (int16_t y = y2 + 1; y <= y3; y++) {
        int16_t ax = x2 + dx1 * (y - y2) / dy1;
        int16_t bx = x1 + dx2 * (y - y1) / dy2;
        
        if (ax > bx) { temp = ax; ax = bx; bx = temp; }
        
        for (int16_t x = ax; x <= bx; x++) {
            oled_draw_point(x, y, mode);
        }
    }
}

// 绘制圆角矩形
void oled_draw_rounded_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t mode)
{
    // 绘制四个角
    oled_draw_arc(x + radius, y + radius, radius, 180, 270, mode); // 左上角
    oled_draw_arc(x + width - radius - 1, y + radius, radius, 270, 360, mode); // 右上角
    oled_draw_arc(x + width - radius - 1, y + height - radius - 1, radius, 0, 90, mode); // 右下角
    oled_draw_arc(x + radius, y + height - radius - 1, radius, 90, 180, mode); // 左下角
    
    // 绘制四条直线
    oled_draw_hline(x + radius, y, width - 2 * radius, mode); // 上边
    oled_draw_hline(x + radius, y + height - 1, width - 2 * radius, mode); // 下边
    oled_draw_vline(x, y + radius, height - 2 * radius, mode); // 左边
    oled_draw_vline(x + width - 1, y + radius, height - 2 * radius, mode); // 右边
}

// 绘制填充圆角矩形
void oled_draw_filled_rounded_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t mode)
{
    // 填充中间矩形区域
    oled_draw_filled_rectangle(x + radius, y, width - 2 * radius, height, mode);
    oled_draw_filled_rectangle(x, y + radius, width, height - 2 * radius, mode);
    
    // 填充四个角的圆形区域
    oled_draw_filled_circle(x + radius, y + radius, radius, mode); // 左上角
    oled_draw_filled_circle(x + width - radius - 1, y + radius, radius, mode); // 右上角
    oled_draw_filled_circle(x + width - radius - 1, y + height - radius - 1, radius, mode); // 右下角
    oled_draw_filled_circle(x + radius, y + height - radius - 1, radius, mode); // 左下角
}

// 绘制圆弧
void oled_draw_arc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t end_angle, uint8_t mode)
{
    // 确保角度在0-360范围内
    start_angle %= 360;
    end_angle %= 360;
    
    if (start_angle > end_angle) {
        uint16_t temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }
    
    // 使用Bresenham圆算法绘制圆弧
    int16_t x0 = 0;
    int16_t y0 = radius;
    int16_t d = 3 - 2 * radius;
    
    while (x0 <= y0) {
        // 检查每个点是否在角度范围内
        for (int16_t i = 0; i < 8; i++) {
            int16_t px, py;
            
            switch (i) {
                case 0: px = x0; py = y0; break;
                case 1: px = y0; py = x0; break;
                case 2: px = -y0; py = x0; break;
                case 3: px = -x0; py = y0; break;
                case 4: px = -x0; py = -y0; break;
                case 5: px = -y0; py = -x0; break;
                case 6: px = y0; py = -x0; break;
                case 7: px = x0; py = -y0; break;
                default: px = 0; py = 0; break;
            }
            
            // 计算当前点的角度
            int16_t angle = (int16_t)(atan2(py, px) * 180.0 / 3.14159);
            if (angle < 0) angle += 360;
            
            // 如果角度在范围内，绘制点
            if (angle >= start_angle && angle <= end_angle) {
                oled_draw_point(x + px, y + py, mode);
            }
        }
        
        if (d < 0) {
            d = d + 4 * x0 + 6;
        } else {
            d = d + 4 * (x0 - y0) + 10;
            y0--;
        }
        x0++;
    }
}

// ==================== 进度条和仪表功能 ====================

// 绘制水平进度条
void oled_draw_horizontal_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t mode)
{
    // 绘制边框
    oled_draw_rectangle(x, y, x + width - 1, y + height - 1, mode);
    
    // 计算填充宽度
    uint8_t fill_width = (width - 2) * progress / 100;
    
    // 绘制填充部分
    if (fill_width > 0) {
        oled_draw_filled_rectangle(x + 1, y + 1, x + fill_width, y + height - 2, mode);
    }
}

// 绘制垂直进度条
void oled_draw_vertical_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t mode)
{
    // 绘制边框
    oled_draw_rectangle(x, y, x + width - 1, y + height - 1, mode);
    
    // 计算填充高度
    uint8_t fill_height = (height - 2) * progress / 100;
    
    // 绘制填充部分
    if (fill_height > 0) {
        oled_draw_filled_rectangle(x + 1, y + height - 1 - fill_height, x + width - 2, y + height - 2, mode);
    }
}

// ==================== 高级文本功能 ====================

// 绘制居中文本
void oled_show_string_centered(uint8_t y, uint8_t *str, uint8_t size)
{
    // 计算字符串宽度
    uint8_t char_width;
    switch (size) {
        case 8: char_width = 6; break;
        case 12: char_width = 8; break;  // 修正：12号字体宽度应为8
        case 16: char_width = 8; break;
        default: char_width = 12; break;
    }
    
    uint8_t str_len = 0;
    uint8_t *p = str;
    
    while (*p++) str_len++;
    
    uint8_t total_width = str_len * char_width;
    uint8_t screen_width = (oled_get_rotation() == R0 || oled_get_rotation() == R2) ? 80 : 128;
    
    // 计算居中位置
    uint8_t x = (screen_width - total_width) / 2;
    
    // 边界检查：确保x坐标不会溢出
    if (x >= screen_width) x = 0;
    
    // 绘制文本
    oled_show_string(x, y, str, size);
}

// 绘制右对齐文本
void oled_show_string_right_aligned(uint8_t x, uint8_t y, uint8_t *str, uint8_t size)
{
    // 计算字符串宽度
    uint8_t char_width = (size == 8) ? 6 : (size == 12) ? 6 : (size == 16) ? 8 : 12;
    uint8_t str_len = 0;
    uint8_t *p = str;
    
    while (*p++) str_len++;
    
    uint8_t total_width = str_len * char_width;
    
    // 计算右对齐位置
    uint8_t start_x = x - total_width;
    
    // 绘制文本
    oled_show_string(start_x, y, str, size);
}

// ==================== 动画和特效功能 ====================

// 淡入效果
void oled_fade_in(uint16_t duration_ms)
{
    uint8_t step_delay = duration_ms / 256;
    
    for (uint16_t i = 0; i <= 255; i++) {
        oled_set_brightness(i);
        HAL_Delay(step_delay);
    }
}

// 淡出效果
void oled_fade_out(uint16_t duration_ms)
{
    uint8_t step_delay = duration_ms / 256;
    
    for (uint16_t i = 255; i > 0; i--) {
        oled_set_brightness(i);
        HAL_Delay(step_delay);
    }
    oled_set_brightness(0);
}

// ==================== 自动亮度控制功能 ====================

// 自动亮度增加
void oled_auto_brightness_increase(uint8_t step, uint16_t delay_ms)
{
    uint8_t current = oled_get_brightness();
    
    for (uint8_t brightness = current; brightness <= 0xFF; brightness += step) {
        oled_set_brightness(brightness);
        HAL_Delay(delay_ms);
    }
    
    // 确保最终亮度为最大值
    oled_set_brightness(0xFF);
}

// 自动亮度减少
void oled_auto_brightness_decrease(uint8_t step, uint16_t delay_ms)
{
    uint8_t current = oled_get_brightness();
    
    for (uint8_t brightness = current; brightness >= 0x00; brightness -= step) {
        oled_set_brightness(brightness);
        HAL_Delay(delay_ms);
    }
    
    // 确保最终亮度为最小值
    oled_set_brightness(0x00);
}

// 呼吸灯效果
void oled_auto_brightness_breathing(uint8_t min_brightness, uint8_t max_brightness, uint16_t cycle_time_ms)
{
    // 计算每个亮度级别的延迟时间
    uint16_t steps = max_brightness - min_brightness;
    uint16_t delay_per_step = cycle_time_ms / (steps * 2); // 上下各一次
    
    if (delay_per_step < 10) {
        delay_per_step = 10; // 最小延迟10ms确保显示稳定
    }
    
    // 从最小亮度平滑增加到最大亮度
    for (uint8_t brightness = min_brightness; brightness <= max_brightness; brightness++) {
        oled_set_brightness(brightness);
        HAL_Delay(delay_per_step);
    }
    
    // 从最大亮度平滑减小到最小亮度
    for (uint8_t brightness = max_brightness; brightness >= min_brightness; brightness--) {
        oled_set_brightness(brightness);
        HAL_Delay(delay_per_step);
    }
    
    // 恢复默认亮度，避免停留在最小亮度
    oled_set_brightness(0x81);
}

// ==================== 任意角度旋转支持 ====================

// 任意角度旋转的X坐标转换（优化版本，使用查找表）
uint16_t oled_rotate_x_any(uint16_t angle, uint16_t x, uint16_t y, uint16_t center_x, uint16_t center_y) {
    // 将角度限制在0~359度范围内
    uint16_t normalized_angle = angle % 360;
    
    // 直接使用预计算的查找表（整数运算，避免浮点数）
    int16_t sin_val = sin_table[normalized_angle];
    int16_t cos_val = cos_table[normalized_angle];
    
    // 计算相对于旋转中心的坐标
    int32_t dx = (int32_t)x - (int32_t)center_x;
    int32_t dy = (int32_t)y - (int32_t)center_y;
    
    // 应用旋转矩阵：x' = x*cos - y*sin
    // 注意：查找表的值已经乘以了1000（对应sin(90°)=1），使用更精确的整数除法
    int32_t rotated_x = ((dx * cos_val - dy * sin_val) + 500) / 1000 + center_x;
    
    return (uint16_t)rotated_x;
}

// 任意角度旋转的Y坐标转换（优化版本，使用查找表）
uint16_t oled_rotate_y_any(uint16_t angle, uint16_t x, uint16_t y, uint16_t center_x, uint16_t center_y) {
    // 将角度限制在0~359度范围内
    uint16_t normalized_angle = angle % 360;
    
    // 直接使用预计算的查找表（整数运算，避免浮点数）
    int16_t sin_val = sin_table[normalized_angle];
    int16_t cos_val = cos_table[normalized_angle];
    
    // 计算相对于旋转中心的坐标
    int32_t dx = (int32_t)x - (int32_t)center_x;
    int32_t dy = (int32_t)y - (int32_t)center_y;
    
    // 应用旋转矩阵：y' = x*sin + y*cos
    // 注意：查找表的值已经乘以了1000（对应sin(90°)=1），使用更精确的整数除法
    int32_t rotated_y = ((dx * sin_val + dy * cos_val) + 500) / 1000 + center_y;
    
    return (uint16_t)rotated_y;
}

// 优化版本的任意角度直线绘制函数
void oled_draw_line_any(uint16_t angle, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t mode)
{
    // 如果角度是90度的倍数，使用更快的90度旋转函数
    if (angle % 90 == 0) {
        rotation_angle_t simple_angle;
        switch (angle % 360) {
            case 0: simple_angle = R0; break;
            case 90: simple_angle = R1; break;
            case 180: simple_angle = R2; break;
            case 270: simple_angle = R3; break;
            default: simple_angle = R0; break;
        }
        
        // 临时保存当前旋转状态
        rotation_angle_t saved_rotation = oled_get_rotation();
        
        // 设置临时旋转角度
        oled_set_rotation(simple_angle);
        
        // 使用标准直线绘制函数（更快）
        oled_draw_line((uint8_t)x1, (uint8_t)y1, (uint8_t)x2, (uint8_t)y2, mode);
        
        // 恢复原始旋转状态
        oled_set_rotation(saved_rotation);
        return;
    }
    
    // 对于非90度倍数的角度，使用优化后的旋转算法
    uint16_t center_x = (x1 + x2) / 2;
    uint16_t center_y = (y1 + y2) / 2;
    
    // 边界检查：确保线段不会超出屏幕
    uint8_t screen_width, screen_height;
    rotation_angle_t current_rot = oled_get_rotation();
    
    switch(current_rot) {
        case R0:
        case R2:
            screen_width = 80;   // 80×128模式
            screen_height = 128;
            break;
        case R1:
        case R3:
            screen_width = 128;  // 128×80模式
            screen_height = 80;
            break;
        default:
            screen_width = 80;
            screen_height = 128;
            break;
    }
    
    // 预计算旋转参数（避免重复计算）
    uint16_t normalized_angle = angle % 360;
    int16_t sin_val = sin_table[normalized_angle];
    int16_t cos_val = cos_table[normalized_angle];
    
    // 使用Bresenham直线算法绘制旋转后的线段
    int16_t dx = abs((int16_t)x2 - (int16_t)x1);
    int16_t dy = abs((int16_t)y2 - (int16_t)y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    
    uint16_t current_x = x1;
    uint16_t current_y = y1;
    
    while(1) {
        // 直接计算旋转后的坐标（避免函数调用开销）
        int32_t dx_local = (int32_t)current_x - (int32_t)center_x;
        int32_t dy_local = (int32_t)current_y - (int32_t)center_y;
        
        // 使用更精确的整数除法，避免精度损失
        int32_t rotated_x = ((dx_local * cos_val - dy_local * sin_val) + 500) / 1000 + center_x;
        int32_t rotated_y = ((dx_local * sin_val + dy_local * cos_val) + 500) / 1000 + center_y;
        
        // 边界检查
        if (rotated_x >= 0 && rotated_x < screen_width && 
            rotated_y >= 0 && rotated_y < screen_height) {
            oled_draw_point((uint8_t)rotated_x, (uint8_t)rotated_y, mode);
        }
        
        if(current_x == x2 && current_y == y2) break;
        
        int16_t e2 = 2 * err;
        if(e2 > -dy) {
            err -= dy;
            current_x += sx;
        }
        if(e2 < dx) {
            err += dx;
            current_y += sy;
        }
    }
}

// ==================== 调试和诊断功能 ====================

// 绘制测试图案
void oled_draw_test_pattern(void)
{
    oled_clear();
    
    // 绘制网格
    for (uint8_t i = 0; i < 80; i += 10) {
        oled_draw_vline(i, 0, 127, 1);
    }
    for (uint8_t i = 0; i < 128; i += 10) {
        oled_draw_hline(0, i, 79, 1);
    }
    
    // 绘制基本图形
    oled_draw_circle(40, 64, 20, 1);
    oled_draw_rectangle(10, 10, 30, 30, 1);
    oled_draw_triangle(60, 10, 70, 30, 50, 30, 1);
    
    // 显示文本
    oled_show_string(5, 90, "SH1107 Test", 8);
    oled_show_string(5, 100, "Pattern", 8);
    
    oled_refresh();
}

// ==================== 亮度控制基础功能 ====================

// 全局亮度级别设置
#define MAX_BRIGHTNESS_LEVEL 100  // 最大亮度级别（0-99）

// 非线性亮度映射函数（符合人眼感知特性）
// 使用对数曲线：低级别对应精细调节，高级别对应较大变化
static uint8_t level_to_brightness(uint8_t level)
{
    // 边界检查
    if (level >= MAX_BRIGHTNESS_LEVEL) level = MAX_BRIGHTNESS_LEVEL - 1;
    
    // 使用对数曲线映射：y = a * log(b * x + c) + d
    // 简化版本：使用平方根曲线近似对数特性
    float normalized_level = (float)level / (MAX_BRIGHTNESS_LEVEL - 1);
    
    // 平方根曲线：在低级别提供更精细的控制，高级别变化更快
    float normalized_brightness = sqrtf(normalized_level);
    
    // 转换为0-255范围
    uint8_t brightness = (uint8_t)(normalized_brightness * 255.0f);
    
    // 确保在有效范围内
    if (brightness > 0xFF) brightness = 0xFF;
    
    return brightness;
}

// 反向映射函数：从亮度值获取级别
static uint8_t brightness_to_level(uint8_t brightness)
{
    // 边界检查
    if (brightness > 0xFF) brightness = 0xFF;
    
    // 反向平方根曲线
    float normalized_brightness = (float)brightness / 255.0f;
    float normalized_level = normalized_brightness * normalized_brightness;  // 平方
    
    // 转换为0-99级别
    uint8_t level = (uint8_t)(normalized_level * (MAX_BRIGHTNESS_LEVEL - 1));
    
    // 确保在有效范围内
    if (level >= MAX_BRIGHTNESS_LEVEL) level = MAX_BRIGHTNESS_LEVEL - 1;
    
    return level;
}

void oled_set_brightness_level(uint8_t level)
{
    // 边界检查：确保级别在0-99范围内
    if (level >= MAX_BRIGHTNESS_LEVEL) level = MAX_BRIGHTNESS_LEVEL - 1;
    
    // 使用非线性映射函数将级别转换为OLED亮度值
    uint8_t brightness = level_to_brightness(level);
    
    oled_set_brightness(brightness);
}

uint8_t oled_get_brightness_level(void)
{
    // 使用非线性映射函数从亮度值获取级别
    return brightness_to_level(current_brightness);
}

void oled_set_brightness(uint8_t brightness)
{
    // 边界检查：确保亮度值在0x00-0xFF范围内
    if (brightness > 0xFF) brightness = 0xFF;
    
    // 如果亮度值没有变化，直接返回，避免不必要的操作
    if (current_brightness == brightness) {
        return;
    }
    
    current_brightness = brightness;
    
    // SH1107对比度控制 - 根据数据手册实现
    // 双字节命令：0x81 + 对比度值(0x00-0xFF)
    
    // 1. 设置对比度控制模式 (0x81)和对比度值 (0x00-0xFF)
    // 使用批量命令发送对比度控制命令
    uint8_t contrast_cmds[] = {0x81, brightness};
    oled_wr_cmds(contrast_cmds, sizeof(contrast_cmds));
    
    // 3. 延长所有亮度级别的预充电周期（确保OLED像素充分充电）
    // 预充电周期影响OLED像素的充电时间和亮度响应
    static uint8_t last_precharge_brightness = 0;
    if (abs((int)brightness - (int)last_precharge_brightness) > 0x10) {
        // 为所有亮度级别设置更长的预充电时间
        uint8_t precharge;
        if (brightness < 0x20) {
            // 极低亮度：最长预充电时间确保像素充分充电
            precharge = 0xFF;  // 最长预充电周期
        } else if (brightness < 0x60) {
            // 低亮度：较长预充电时间
            precharge = 0xCF;  // 较长预充电周期
        } else if (brightness < 0xA0) {
            // 中亮度：中等预充电时间
            precharge = 0x8F;  // 中等预充电周期
        } else {
            // 高亮度：标准预充电时间
            precharge = 0x4F;  // 标准预充电周期
        }
        
        // 使用批量命令发送预充电周期命令
        uint8_t precharge_cmds[] = {0xD9, precharge};
        oled_wr_cmds(precharge_cmds, sizeof(precharge_cmds));
        last_precharge_brightness = brightness;
    }
    
    // 重要：对比度控制不需要发送显示ON命令(0xAF)
    // 显示ON命令只在初始化或显示开关时使用
    // 频繁发送显示ON命令会导致不必要的刷新和闪烁
}

uint8_t oled_get_brightness(void)
{
    return current_brightness;
}

// ==================== 屏幕旋转功能（90度倍数） ====================

void oled_set_rotation(rotation_angle_t rotation)
{
    current_rotation = rotation;
    
    // 根据旋转角度设置显示方向
    switch (rotation) {
        case R0:  // 0度旋转（默认方向）
            oled_display_turn(0);  // 正常方向
            break;
        case R1:  // 90度旋转
            oled_display_turn(1);  // 翻转方向
            break;
        case R2:  // 180度旋转
            oled_display_turn(0);  // 正常方向（但需要调整坐标）
            break;
        case R3:  // 270度旋转
            oled_display_turn(1);  // 翻转方向
            break;
        default:
            current_rotation = R0;
            oled_display_turn(0);
            break;
    }
}

rotation_angle_t oled_get_rotation(void)
{
    return current_rotation;
}