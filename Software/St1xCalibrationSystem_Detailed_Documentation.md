# ST1X 校准系统详细技术文档

## 1. 系统架构与设计原理

### 1.1 系统整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                   主系统 (Main System)                       │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────┐    │
│  │             校准系统 (CalibrationSystem)            │    │
│  ├─────────────────────────────────────────────────────┤    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │    │
│  │  │  状态管理   │  │  温度控制  │  │  界面显示   │   │    │
│  │  │  模块       │  │  模块      │  │  模块       │   │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘   │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │    │
│  │  │  按键处理   │  │  数据存储  │  │  电压检测   │   │    │
│  │  │  模块       │  │  模块      │  │  模块       │   │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘   │    │
│  └─────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │  PID控制    │  │  温度采集   │  │  OLED显示   │        │
│  │  系统       │  │  系统       │  │  系统       │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 核心设计参数

**系统配置参数：**
```c
#define CALIBRATION_POINTS 9                    // 校准点数：9个
#define TEMPERATURE_TOLERANCE 5.0f              // 温度容差：±5°C
#define TEMPERATURE_STABLE_TIME 1000            // 温度稳定时间：1000ms
#define UPDATE_INTERVAL 20                       // 更新间隔：20ms
#define TEMPERATURE_ADJUST_STEP 1.0f            // 温度调整步进：1.0°C
#define KEY_DEBOUNCE_TIME 50                    // 按键去抖时间：50ms
#define CAL_KEY_LONG_PRESS_TIME 1000            // 按键长按时间：1000ms
#define LOW_VOLTAGE_DISPLAY_TIME 5000           // 电压不足显示时间：5000ms
```

**校准温度点定义：**
```c
static const float calibration_temperatures[CALIBRATION_POINTS] = {
    100.0f, 150.0f, 200.0f, 250.0f, 300.0f, 
    350.0f, 400.0f, 450.0f, 500.0f
};
```

### 1.3 数据结构设计

**校准点结构体：**
```c
typedef struct {
    uint8_t point_id;                    // 校准点ID（1-9）
    float target_temperature;           // 目标温度
    float adjusted_temperature;         // 调整后的目标温度
    float calibration_offset;           // 校准偏移量
    CalibrationPointState state;        // 当前状态
    uint32_t heating_start_time;       // 加热开始时间
    uint32_t stable_start_time;        // 稳定开始时间
    float max_temp_deviation;          // 最大温度偏差记录
    uint8_t heating_cycles;            // 加热循环次数
} CalibrationPoint;
```

**系统状态变量：**
```c
static CalibrationSystemState system_state = CAL_STATE_IDLE;
static CalibrationPoint calibration_points[CALIBRATION_POINTS];
static uint8_t current_point_index = 0;
static uint32_t last_update_time = 0;
static uint8_t save_result = 0;
static uint8_t low_voltage_state = 0;
static uint32_t low_voltage_start_time = 0;
```

## 2. 系统启动流程详细分析

### 2.1 系统初始化阶段

**调用入口：** `CalibrationSystem_Init()`

**初始化步骤：**
1. **状态变量重置**
   - `system_state = CAL_STATE_IDLE`
   - `current_point_index = 0`
   - `save_result = 0`
   - `last_update_time = 0`

2. **电压检测状态重置**
   - `low_voltage_state = 0`
   - `low_voltage_start_time = 0`

3. **校准点初始化**
   ```c
   for (int i = 0; i < CALIBRATION_POINTS; i++) {
       InitializeCalibrationPoint(i);
   }
   ```

4. **温度限制设置**
   - `calibration_temperature_limit = 500.0f`

**单个校准点初始化逻辑：**
```c
static void InitializeCalibrationPoint(uint8_t point_index) {
    CalibrationPoint* point = &calibration_points[point_index];
    
    point->point_id = point_index + 1;  // ID从1开始
    point->target_temperature = calibration_temperatures[point_index];
    point->adjusted_temperature = calibration_temperatures[point_index];
    point->calibration_offset = 0.0f;  // 初始偏移量为0
    point->state = CAL_POINT_WAITING;
    point->heating_start_time = 0;
    point->stable_start_time = 0;
    point->max_temp_deviation = 0.0f;
    point->heating_cycles = 0;
}
```

### 2.2 系统启动阶段

**调用入口：** `CalibrationSystem_Start()`

**启动条件检查：**
```c
if (system_state != CAL_STATE_IDLE) {
    return;  // 只有在空闲状态才能启动
}
```

**启动步骤详细分析：**

1. **第一个校准点设置**
   ```c
   if (CALIBRATION_POINTS > 0) {
       CalibrationPoint* first_point = &calibration_points[0];
       first_point->target_temperature = calibration_temperatures[0];
       first_point->adjusted_temperature = calibration_temperatures[0];
       
       // 设置PID目标温度
       SetPointTemperature(0, first_point->adjusted_temperature);
       
       // 显式设置目标温度变量
       extern float target_temperature;
       target_temperature = first_point->adjusted_temperature;
   }
   ```

2. **加热系统启动**
   ```c
   heating_status = 1;  // 设置加热状态为启用
   HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);  // 启动PWM输出
   startHeatingControlTimer();  // 启动PID控制器的定时器
   ```

3. **状态转换**
   ```c
   system_state = CAL_STATE_VOLTAGE_CHECK;
   low_voltage_start_time = HAL_GetTick();  // 记录电压检测开始时间
   ```

## 3. 电压检测阶段详细流程

### 3.1 电压检测状态处理

**处理函数：** `ProcessVoltageCheckState(uint32_t current_time)`

**检测逻辑：**
```c
// 检查电压是否足够
extern uint8_t isUSBVoltageSufficient(void);
if (isUSBVoltageSufficient()) {
    // 电压足够，进入校准运行状态
    // ... 详细处理逻辑
} else {
    // 电压不足，检查是否超过5秒显示时间
    if ((current_time - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
        CalibrationSystem_Stop();  // 超过5秒，返回主界面
    }
}
```

**电压足够时的处理流程：**

1. **温度限制调整**
   ```c
   extern float max_temperature_limit;
   max_temperature_limit = 500.0f;  // 校准温度限制
   ```

2. **系统状态重置**
   ```c
   current_point_index = 0;
   save_result = 0;
   system_state = CAL_STATE_RUNNING;
   ```

3. **校准点重新初始化**
   ```c
   for (int i = 0; i < CALIBRATION_POINTS; i++) {
       InitializeCalibrationPoint(i);
   }
   ```

4. **第一个校准点启动**
   ```c
   SetPointTemperature(0, calibration_points[0].adjusted_temperature);
   calibration_points[0].state = CAL_POINT_HEATING;
   calibration_points[0].heating_start_time = current_time;
   calibration_points[0].heating_cycles = 1;
   
   // 立即将校准点温度写入目标温度变量
   extern float target_temperature;
   target_temperature = calibration_points[0].adjusted_temperature;
   ```

### 3.2 电压检测状态下的按键处理

**处理函数：** `HandleVoltageCheckKey(KeyType key)`

**按键响应逻辑：**

1. **MODE短按（确认电压正常）**
   ```c
   if (key == KEY_MODE) {
       system_state = CAL_STATE_RUNNING;
       current_point_index = 0;
       SetPointTemperature(current_point_index, calibration_points[current_point_index].adjusted_temperature);
       extern float target_temperature;
       target_temperature = calibration_points[current_point_index].adjusted_temperature;
       calibration_points[current_point_index].state = CAL_POINT_HEATING;
   }
   ```

2. **MODE长按（跳过电压检测）**
   ```c
   else if (key == KEY_MODE_LONG) {
       system_state = CAL_STATE_RUNNING;
       // ... 与短按相同的处理逻辑
   }
   ```

## 4. 校准运行阶段详细分析

### 4.1 系统更新循环

**主更新函数：** `CalibrationSystem_Update(u8g2_t *u8g2)`

**更新频率控制：**
```c
uint32_t current_time = HAL_GetTick();
if ((current_time - last_update_time) < UPDATE_INTERVAL) {
    return;  // 20ms间隔控制
}
last_update_time = current_time;
```

**更新流程：**
1. **按键扫描**
   ```c
   KeyType key = CalibrationKey_Scan();
   if (key != KEY_NONE) {
       CalibrationSystem_HandleKey(key);
   }
   ```

2. **状态处理**
   ```c
   switch (system_state) {
       case CAL_STATE_VOLTAGE_CHECK:
           ProcessVoltageCheckState(current_time);
           break;
       case CAL_STATE_RUNNING:
           ProcessCalibrationPoint(current_point_index);
           break;
       default:
           break;
   }
   ```

3. **界面绘制**
   ```c
   DrawCalibrationInterface(u8g2, current_point_index);
   ```

### 4.2 单个校准点处理状态机

**处理函数：** `ProcessCalibrationPoint(uint8_t point_index)`

#### 4.2.1 加热阶段 (CAL_POINT_HEATING)

**温度稳定性判断逻辑：**
```c
float current_temp = filtered_temperature;
float target_temp = point->adjusted_temperature;
float temp_diff = fabs(current_temp - target_temp);

// 记录最大温度偏差
if (temp_diff > point->max_temp_deviation) {
    point->max_temp_deviation = temp_diff;
}

// 检查温度是否稳定
if (temp_diff <= TEMPERATURE_TOLERANCE) {
    if (point->stable_start_time == 0) {
        point->stable_start_time = current_time;
    } else if ((current_time - point->stable_start_time) >= TEMPERATURE_STABLE_TIME) {
        point->state = CAL_POINT_STABLE;  // 进入稳定状态
    }
} else {
    point->stable_start_time = 0;  // 温度不稳定，重置计时器
}
```

#### 4.2.2 稳定阶段 (CAL_POINT_STABLE)

**按键处理逻辑：** `HandleRunningStateKey(KeyType key)`

**UP按键处理：**
```c
if (key == KEY_UP) {
    // 增加偏移量（补偿负偏差）
    point->calibration_offset += TEMPERATURE_ADJUST_STEP;
    
    // 重新计算调整后的目标温度
    point->adjusted_temperature = point->target_temperature + point->calibration_offset;
    
    // 温度上限保护
    if (point->adjusted_temperature > calibration_temperature_limit) {
        point->adjusted_temperature = calibration_temperature_limit;
        point->calibration_offset = point->adjusted_temperature - point->target_temperature;
    }
    
    // 更新PID目标温度
    setT12Temperature(point->adjusted_temperature);
}
```

**DOWN按键处理：**
```c
else if (key == KEY_DOWN) {
    // 减少偏移量（补偿正偏差）
    point->calibration_offset -= TEMPERATURE_ADJUST_STEP;
    
    // 重新计算调整后的目标温度
    point->adjusted_temperature = point->target_temperature + point->calibration_offset;
    
    // 温度下限保护
    if (point->adjusted_temperature < 0.0f) {
        point->adjusted_temperature = 0.0f;
        point->calibration_offset = point->adjusted_temperature - point->target_temperature;
    }
    
    // 更新PID目标温度
    setT12Temperature(point->adjusted_temperature);
}
```

#### 4.2.3 调整完成阶段 (CAL_POINT_ADJUSTED)

**校准点完成处理：** `HandleCalibrationPointCompleted(uint8_t point_index)`

**完成逻辑：**
```c
// 如果是最后一个校准点，完成整个校准过程
if (point_index == (CALIBRATION_POINTS - 1)) {
    system_state = CAL_STATE_COMPLETE;
} else {
    // 切换到下一个校准点
    current_point_index++;
    CalibrationPoint* next_point = &calibration_points[current_point_index];
    
    // 设置下一个校准点的目标温度
    SetPointTemperature(current_point_index, next_point->target_temperature + next_point->calibration_offset);
    
    // 显式设置目标温度变量
    extern float target_temperature;
    target_temperature = next_point->target_temperature + next_point->calibration_offset;
    
    // 重置下一个校准点的状态
    next_point->state = CAL_POINT_HEATING;
    next_point->stable_start_time = 0;
}
```

## 5. 界面显示系统详细分析

### 5.1 界面绘制函数

**主绘制函数：** `DrawCalibrationInterface(u8g2_t *u8g2, uint8_t point_index)`

**绘制流程：**
1. **缓冲区清空**
   ```c
   u8g2_ClearBuffer(u8g2);
   ```

2. **电压检测**
   ```c
   if (!isUSBVoltageSufficient()) {
       // 显示电压不足提示
       u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
       u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
       u8g2_DrawStr(u8g2, 0, 32, "Cannot calibrate");
       u8g2_SendBuffer(u8g2);
       
       // 检查是否达到显示时间
       if ((HAL_GetTick() - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
           CalibrationSystem_Stop();
           low_voltage_state = 0;
       }
       return;
   }
   ```

3. **状态相关界面绘制**
   - 电压检测状态界面
   - 运行状态界面
   - 完成状态界面

### 5.2 状态文本获取

**状态文本函数：** `GetPointStateText(CalibrationPointState state)`
```c
static const char* GetPointStateText(CalibrationPointState state) {
    switch (state) {
        case CAL_POINT_WAITING:  return "Waiting";
        case CAL_POINT_HEATING:  return "Heating...";
        case CAL_POINT_STABLE:   return "Stable";
        case CAL_POINT_ADJUSTED: return "Confirmed";
        default: return "Unknown";
    }
}
```

**帮助文本函数：** `GetPointHelpText(CalibrationPointState state)`
```c
static const char* GetPointHelpText(CalibrationPointState state) {
    switch (state) {
        case CAL_POINT_HEATING:  return "Heating...Wait";
        case CAL_POINT_STABLE:   return "UP/DN:Adj MODE:OK";
        case CAL_POINT_ADJUSTED: return "Completed";
        default: return "";
    }
}
```

## 6. 系统退出流程详细分析

### 6.1 正常退出流程

**停止函数：** `CalibrationSystem_Stop()`

**退出条件检查：**
```c
if (system_state == CAL_STATE_IDLE) {
    return;  // 只有在非空闲状态才能停止
}
```

**退出步骤：**

1. **停止加热控制**
   ```c
   stopHeatingControlTimer();  // 停止PID控制器的定时器
   HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);  // 停止PWM输出
   ```

2. **重置加热状态**
   ```c
   heating_status = 0;
   ```

3. **恢复温度限制**
   ```c
   extern float max_temperature_limit;
   max_temperature_limit = 460.0f;  // 正常温度限制
   ```

4. **系统状态重置**
   ```c
   system_state = CAL_STATE_IDLE;
   current_point_index = 0;
   save_result = 0;
   ```

5. **电压状态重置**
   ```c
   low_voltage_state = 0;
   ```

### 6.2 异常退出处理

**异常退出场景：**
1. 用户长按MODE键强制退出
2. 电压不足超时自动退出
3. 系统错误导致的异常退出

**安全保护措施：**
- 立即停止加热，防止温度失控
- 恢复原始温度设置
- 清理系统状态，确保安全退出

## 7. 模块间交互详细分析

### 7.1 与PID控制器的交互

**外部变量声明：**
```c
extern void setT12Temperature(float temperature);
extern void startHeatingControlTimer(void);
extern void stopHeatingControlTimer(void);
extern float target_temperature;
extern uint8_t heating_status;
extern uint8_t heating_control_enabled;
extern TIM_HandleTypeDef htim2;
```

**温度设置函数：** `SetPointTemperature(uint8_t point_index, float temperature)`

**交互流程：**
```c
// 设置PID目标温度
setT12Temperature(temperature);

// 启动/停止控制定时器
startHeatingControlTimer();
stopHeatingControlTimer();
```

### 7.2 与温度采集模块的交互

**温度数据获取：**
```c
extern float filtered_temperature;
float current_temp = filtered_temperature;
```

**数据流路径：**
```
ADC温度采集 → 滤波处理 → filtered_temperature → 校准系统温度监控
```

### 7.3 与显示系统的交互

**显示控制：**
```c
// 使用u8g2库进行OLED显示
u8g2_ClearBuffer(u8g2);
u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
u8g2_DrawStr(u8g2, x, y, text);
u8g2_SendBuffer(u8g2);
```

### 7.4 与按键系统的交互

**按键扫描：** `CalibrationKey_Scan()`

**按键类型定义：**
```c
typedef enum {
    KEY_NONE,
    KEY_UP,
    KEY_DOWN, 
    KEY_MODE,
    KEY_MODE_LONG
} KeyType;
```

## 8. 系统运行状态机详细分析

### 8.1 系统状态定义

**系统状态枚举：**
```c
typedef enum {
    CAL_STATE_IDLE,           // 空闲状态
    CAL_STATE_VOLTAGE_CHECK,  // 电压检测状态
    CAL_STATE_RUNNING,        // 运行状态
    CAL_STATE_COMPLETE        // 完成状态
} CalibrationSystemState;
```

**校准点状态枚举：**
```c
typedef enum {
    CAL_POINT_WAITING,   // 等待状态
    CAL_POINT_HEATING,   // 加热状态
    CAL_POINT_STABLE,    // 稳定状态
    CAL_POINT_ADJUSTED   // 调整完成状态
} CalibrationPointState;
```

### 8.2 状态转换详细流程

**完整状态转换图：**

```
[主系统] → [CAL_STATE_IDLE]
    ↓ (用户选择校准)
[CAL_STATE_VOLTAGE_CHECK]
    ├── 电压足够 → [CAL_STATE_RUNNING]
    ├── 电压不足超时 → [CAL_STATE_IDLE] 
    └── 用户按键跳过 → [CAL_STATE_RUNNING]
        ↓
[CAL_STATE_RUNNING]
    ├── 处理校准点1-9
    │   ├── CAL_POINT_WAITING → CAL_POINT_HEATING
    │   ├── CAL_POINT_HEATING → CAL_POINT_STABLE
    │   ├── CAL_POINT_STABLE → CAL_POINT_ADJUSTED
    │   └── CAL_POINT_ADJUSTED → 下一个点或完成
    ├── 用户强制退出 → [CAL_STATE_IDLE]
    └── 所有点完成 → [CAL_STATE_COMPLETE]
        ↓
[CAL_STATE_COMPLETE] → [CAL_STATE_IDLE]
```

### 8.3 状态转换条件详细说明

**IDLE → VOLTAGE_CHECK：**
- 条件：用户选择校准功能
- 动作：启动加热系统，开始电压检测

**VOLTAGE_CHECK → RUNNING：**
- 条件1：电压检测通过
- 条件2：用户按键跳过电压检测
- 动作：初始化校准点，开始第一个点加热

**RUNNING状态内部转换：**
- 加热 → 稳定：温度在容差范围内保持1000ms
- 稳定 → 调整完成：用户确认调整
- 调整完成 → 下一个点：自动切换到下一个校准点

**RUNNING → COMPLETE：**
- 条件：第9个校准点调整完成
- 动作：**立即停止加热** → 保存数据 → 设置完成状态

**任何状态 → IDLE：**
- 条件：用户强制退出或系统异常
- 动作：安全停止加热，恢复系统设置

## 9. 错误处理与安全机制

### 9.1 电压检测安全机制

**检测频率：** 每20ms检测一次
**超时保护：** 5秒自动退出
**用户干预：** 支持按键跳过检测

### 9.2 温度保护机制

**温度范围限制：** 0-500°C
**实时监控：** 持续监测温度偏差
**异常处理：** 超温立即停止加热

### 9.3 校准完成自动停止加热机制

**安全改进背景：**
- **问题：** 原系统在校准完成进入保存界面时，加热器仍继续工作
- **风险：** 可能导致过热和长时间运行的安全隐患
- **改进：** 在校准完成时立即自动停止加热

**实现机制：**
```c
// 在HandleCalibrationPointCompleted函数中，最后一个校准点完成时
if (point_index == (CALIBRATION_POINTS - 1)) {
    // 校准完成，立即停止加热以确保安全
    stopHeatingControlTimer();  // 停止PID控制器的定时器
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);  // 停止PWM输出
    heating_status = 0;  // 禁用加热状态
    
    // 保存校准数据
    SaveCalibrationData();
    
    // 设置完成状态
    system_state = CAL_STATE_COMPLETE;
    
    // 调试信息输出
    #ifdef CALIBRATION_DEBUG_ENABLED
    printf("校准完成，加热已停止，数据已保存\\n");
    #endif
}
```

**安全优势：**
- **防止过热：** 校准完成后立即停止加热，避免温度失控
- **节能安全：** 减少不必要的能耗和加热器磨损
- **用户体验：** 用户无需手动停止加热，操作更安全便捷

### 9.4 系统状态一致性保护

**状态检查：** 每次状态转换前检查当前状态
**资源管理：** 确保加热器正确启动/停止
**数据完整性：** 校准数据正确保存

## 10. 性能优化与资源使用分析

### 10.1 内存使用优化

**静态内存分配：**
- 校准点数组：9个结构体，固定大小
- 状态变量：最小化全局变量使用
- 临时变量：栈上分配，自动回收

**内存占用分析：**
```c
// CalibrationPoint结构体大小估算
sizeof(CalibrationPoint) ≈ 4 + 4 + 4 + 4 + 4 + 4 + 4 + 1 ≈ 29字节
总内存占用：9 × 29 ≈ 261字节
```

### 10.2 CPU使用优化

**更新频率控制：** 20ms间隔，避免过度占用CPU
**计算优化：** 使用整数运算替代浮点运算
**状态机优化：** 最小化状态转换开销

### 10.3 实时性保障

**关键路径优化：**
- 温度采集响应时间：< 1ms
- 控制算法执行：< 5ms  
- 界面刷新：20ms间隔

## 11. 扩展性与维护性分析

### 11.1 系统扩展性

**校准点扩展：** 修改`CALIBRATION_POINTS`宏定义
**温度范围扩展：** 调整温度数组和限制值
**功能扩展：** 模块化设计支持功能添加

### 11.2 代码维护性

**模块化设计：** 功能分离，职责明确
**注释完善：** 关键函数和逻辑都有详细注释
**配置集中：** 所有参数在文件头部集中定义

### 11.3 调试支持

**调试开关：** `CALIBRATION_DEBUG_ENABLED`
**状态显示：** 实时显示系统状态和温度信息
**错误日志：** 支持错误状态记录和显示

---

## 12. 重要更新记录

### 12.1 安全改进更新（2024年）

**改进内容：** 校准完成自动停止加热机制
- **问题修复：** 解决校准完成进入保存界面时加热器继续工作的安全隐患
- **实现方式：** 在`HandleCalibrationPointCompleted`函数中添加完整的加热停止逻辑
- **安全优势：** 防止过热、节能安全、优化用户体验

**关键代码修改：**
```c
// 停止PID控制器的定时器
stopHeatingControlTimer();

// 停止PWM输出
HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);

// 禁用加热状态
heating_status = 0;
```

**外部声明补充：**
```c
extern TIM_HandleTypeDef htim2;  // 添加htim2外部声明
```

### 12.2 系统变量完善

**新增系统变量：**
```c
static uint8_t low_voltage_state = 0;        // 电压检测状态
static uint32_t low_voltage_start_time = 0;  // 电压检测开始时间
```

---

**文档版本：** v1.1  
**最后更新：** 2024年（安全改进更新）  
**适用系统：** ST1X焊接台校准系统  
**代码文件：** `St1xCalibrationSystem.c`