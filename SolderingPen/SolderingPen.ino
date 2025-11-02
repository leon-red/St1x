// 烙铁笔控制程序
//
// 基于ATmega328控制的烙铁笔，适用于Hakko T12烙铁头
//
// 本版本代码实现的功能：
// - 烙铁头温度测量
// - 加热器的直接控制或PID控制
// - 通过按钮进行温度控制
// - 短按第一个按钮进入增强模式
// - 长按第一个按钮进入设置菜单
// - 通过陀螺仪检测手柄移动
// - 烙铁未连接检测（通过识别无效温度读数）
// - 如果烙铁未使用（移动检测）则进入定时休眠/关机模式
// - 测量输入电压、Vcc和ATmega内部温度
// - 在OLED上显示信息
// - 蜂鸣器提示
// - 校准和管理不同的烙铁头
// - 将用户设置存储到EEPROM中
// - 烙铁头更换检测
// - 可使用N沟道或P沟道MOSFET
//
// 电源应在6V至20V范围内。支持PD、QC和其他快速充电协议。
//
// 校准需要烙铁头温度计。为获得最佳效果，请在打开烙铁站后至少等待三分钟再开始校准过程。
//
// 控制器：ATmega328p
// 核心：Barebones ATmega (https://github.com/carlosefr/atmega)
// 时钟速度：16 MHz外部晶振
//
// 建议不要使用引导加载程序！
//
// 感谢代码贡献者：
// - John Glavinos, https://youtu.be/4YDcWfOQmz4
// - createskyblue, https://github.com/createskyblue
// - TaaraLabs, https://github.com/TaaraLabs
// - wagiminator, https://github.com/wagiminator



// 库文件包含
#include <U8g2lib.h>             // OLED显示库：https://github.com/olikraus/u8g2

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <PID_v1.h>             // PID控制库：https://github.com/wagiminator/ATmega-Soldering-Station/blob/master/software/libraries/Arduino-PID-Library.zip
// (旧版cpp版本：https://github.com/mblythe86/C-PID-Library/tree/master/PID_v1)
#include <EEPROM.h>             // 用于将用户设置存储到EEPROM中
#include <avr/sleep.h>          // 用于ADC采样期间进入睡眠模式

#include "LIS2DW12Sensor.h"

// 固件版本号
#define VERSION       "v1.5"

// MOSFET类型定义
#define P_MOSFET                // P沟道MOSFET或N沟道MOSFET

// OLED控制器类型
#define SH1107                 // SH1107 OLED控制器
typedef u8g2_uint_t u8g_uint_t;

// 旋转编码器类型
#define ROTARY_TYPE   0         // 0: 每步2个增量；1: 每步4个增量（默认）
#define BUTTON_DELAY  5

// 引脚定义
#define SENSOR_PIN    A0        // 烙铁头温度传感器引脚
#define VIN_PIN       A1        // 输入电压检测引脚
#define BUZZER_PIN     5        // 蜂鸣器引脚
#define BUTTON_PIN     6        // 旋转编码器开关引脚
#define BUTTON_P_PIN   7        // 旋转编码器引脚1
#define BUTTON_N_PIN   8        // 旋转编码器引脚2
#define CONTROL_PIN    9        // 加热器MOSFET PWM控制引脚
#define SWITCH_PIN    10        // 手柄振动开关引脚

// 默认温度控制值（推荐焊接温度：300-380°C）
#define TEMP_MIN      150       // 最小可选温度
#define TEMP_MAX      400       // 最大可选温度
#define TEMP_DEFAULT  320       // 默认启动设定点温度
#define TEMP_SLEEP    150       // 休眠模式温度
#define TEMP_BOOST     50       // 增强模式温度增加值
#define TEMP_STEP      10       // 旋转编码器温度变化步长

// 默认烙铁头温度校准值
#define TEMP200       216       // ADC值为200时的温度
#define TEMP280       308       // ADC值为280时的温度
#define TEMP360       390       // ADC值为360时的温度
#define TEMPCHP       30        // 校准时芯片温度
#define TIPMAX        8         // 最大烙铁头数量
#define TIPNAMELENGTH 6         // 烙铁头名称最大长度（包括终止符）
#define TIPNAME       "BC1.5"   // 默认烙铁头名称

// 默认定时器值（0 = 禁用）
#define TIME2SLEEP    30        // 进入休眠模式的时间（分钟）
#define TIME2OFF       5        // 关闭加热器的时间（分钟）
#define TIMEOFBOOST   40        // 保持在增强模式的时间（秒）
#define LISTHRESHOLD  15        // LIS传感器阈值

// 控制参数
#define TIME2SETTLE   950       // 运放输出稳定时间（微秒）
#define SMOOTHIE      0.05      // 运放输出平滑因子（1=不平滑；0.05默认）
#define PID_ENABLE    false     // 启用PID控制
#define BEEP_ENABLE   true      // 启用/禁用蜂鸣器
#define MAINSCREEN    1         // 主屏幕类型（0: 大数字显示；1: 更多信息显示）

// EEPROM标识符
#define EEPROM_IDENT  0xE76C   // 用于识别EEPROM是否由本程序写入

// MOSFET控制定义
#if defined (P_MOSFET)         // P沟道MOSFET
#define HEATER_ON   255
#define HEATER_OFF  0
#define HEATER_PWM  255 - Output
#elif defined (N_MOSFET)       // N沟道MOSFET
#define HEATER_ON   0
#define HEATER_OFF  255
#define HEATER_PWM  Output
#else
#error 错误的MOSFET类型！
#endif

// 定义激进和保守的PID调谐参数
double aggKp = 11, aggKi = 0.5, aggKd = 1;
double consKp = 11, consKi = 3, consKd = 5;

// 可由用户更改并存储在EEPROM中的默认值
uint16_t  DefaultTemp   = TEMP_DEFAULT;
uint16_t  SleepTemp     = TEMP_SLEEP;
uint8_t   BoostTemp     = TEMP_BOOST;
uint8_t   time2sleep    = TIME2SLEEP;
uint8_t   time2off      = TIME2OFF;
uint8_t   timeOfBoost   = TIMEOFBOOST;
uint8_t   MainScrType   = MAINSCREEN;
bool      PIDenable     = PID_ENABLE;
bool      beepEnable    = BEEP_ENABLE;
uint8_t   LISthreshold  = LISTHRESHOLD;

// 烙铁头的默认值
uint16_t  CalTemp[TIPMAX][4] = {TEMP200, TEMP280, TEMP360, TEMPCHP};
char      TipName[TIPMAX][TIPNAMELENGTH] = {TIPNAME};
uint8_t   CurrentTip   = 0;
uint8_t   NumberOfTips = 1;

// 菜单项定义
const char *SetupItems[]       = { "设置菜单", "烙铁头设置", "温度设置",
                                   "定时器设置", "控制类型", "主屏幕",
                                   "蜂鸣器", "信息", "返回"
                                 };
const char *TipItems[]         = { "烙铁头:", "更换烙铁头", "校准烙铁头",
                                   "重命名烙铁头", "删除烙铁头", "添加新烙铁头", "返回"
                                 };
const char *TempItems[]        = { "温度设置", "默认温度", "休眠温度",
                                   "增强温度", "返回"
                                 };
const char *TimerItems[]       = { "定时器设置", "休眠定时器", "关机定时器",
                                   "增强定时器", "IMU阈值", "返回"
                                 };
const char *ControlTypeItems[]  = { "控制类型", "直接控制", "PID控制" };
const char *MainScreenItems[]   = { "主屏幕", "大数字显示", "更多信息" };
const char *StoreItems[]        = { "保存设置？", "否", "是" };
const char *SureItems[]         = { "确定吗？", "否", "是" };
const char *BuzzerItems[]       = { "蜂鸣器", "禁用", "启用" };
const char *DefaultTempItems[]  = { "默认温度", "摄氏度" };
const char *SleepTempItems[]    = { "休眠温度", "摄氏度" };
const char *BoostTempItems[]    = { "增强温度", "摄氏度" };
const char *SleepTimerItems[]   = { "休眠定时器", "秒" };
const char *LISthresholdItems[] = { "IMU阈值", "mg" };
const char *OffTimerItems[]     = { "关机定时器", "分钟" };
const char *BoostTimerItems[]   = { "增强定时器", "秒" };
const char *DeleteMessage[]     = { "警告", "你不能", "删除你的", "最后一个烙铁头！" };
const char *MaxTipMessage[]     = { "警告", "你已达到", "最大数量", "的烙铁头！" };

// 引脚变化中断相关变量
volatile uint8_t  a0, b0, c0, d0;
volatile bool     ab0;
volatile int      count, countMin, countMax, countStep;
volatile bool     handleMoved;

// 温度控制相关变量
uint16_t  SetTemp, ShowTemp, gap, Step;
double    Input, Output, Setpoint, RawTemp, CurrentTemp, ChipTemp;

// 电压读数相关变量
uint16_t  Vcc, Vin;

// 状态变量
bool      inSleepMode = false;
bool      inOffMode   = false;
bool      inBoostMode = false;
bool      inCalibMode = false;
bool      isWorky     = true;
bool      beepIfWorky = true;
bool      TipIsPresent = true;

// 定时相关变量
uint32_t  sleepmillis;
uint32_t  boostmillis;
uint32_t  buttonmillis;
uint8_t   goneMinutes;
uint8_t   goneSeconds;
uint8_t   SensorCounter = 255;

// 指定变量指针和初始PID调谐参数
PID ctrl(&Input, &Output, &Setpoint, aggKp, aggKi, aggKd, REVERSE);

// 根据OLED控制器设置u8g对象
#if defined (SSD1306)
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST);
#elif defined (SH1106)
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_FAST | U8G_I2C_OPT_NO_ACK);
#elif defined (SH1107)
U8G2_SH1107_64X128_1_HW_I2C u8g(U8G2_R1, A3);
#else
#error 错误的OLED控制器类型！
#endif

// 用于drawStr的缓冲区
char F_Buffer[20];

// LIS2DW12传感器对象
LIS2DW12Sensor Accelero(&Wire, LIS2DW12_I2C_ADD_L);
LIS2DW12_Event_Status_t state;
float lastLISTmp = 0;
float newLISTmp = 0;
uint8_t LISTmpTime = 0;


void setup() {
  // 设置引脚模式
  pinMode(SENSOR_PIN,   INPUT);
  pinMode(VIN_PIN,      INPUT);
  pinMode(BUZZER_PIN,   OUTPUT);
  pinMode(CONTROL_PIN,  OUTPUT);
  pinMode(BUTTON_P_PIN, INPUT_PULLUP);
  pinMode(BUTTON_N_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN,   INPUT_PULLUP);
  pinMode(SWITCH_PIN,   INPUT_PULLUP);

  analogWrite(CONTROL_PIN, HEATER_OFF); // 关闭加热器
  digitalWrite(BUZZER_PIN, LOW);        // 蜂鸣器不使用时必须为低电平

  // 设置ADC
  ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);  // 设置ADC预分频器为128
  ADCSRA |= bit (ADIE);                 // 启用ADC中断
  interrupts ();                        // 启用全局中断

  // // 设置旋转编码器的引脚变化中断
  // PCMSK0 = bit (PCINT0);                // 在Pin8上配置引脚变化中断
  // PCICR  = bit (PCIE0);                 // 启用引脚变化中断
  // PCIFR  = bit (PCIF0);                 // 清除中断标志

  // 设置LIS传感器
  Wire.begin();
  Accelero.begin();
  Accelero.Enable_X();
  Accelero.WriteReg(LIS2DW12_CTRL6, 0x04);
  Accelero.WriteReg(LIS2DW12_CTRL1, 0x17);
  Accelero.Set_FIFO_Mode(LIS2DW12_STREAM_MODE);

  // 准备并启动OLED
  u8g.begin();
  //  if      ( u8g.getMode() == U8G_MODE_R3G3B2 )   u8g.setColorIndex(255);
  //  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) u8g.setColorIndex(3);
  //  else if ( u8g.getMode() == U8G_MODE_BW )       u8g.setColorIndex(1);
  //  else if ( u8g.getMode() == U8G_MODE_HICOLOR )  u8g.setHiColorByRGB(255,255,255);

  // 从EEPROM获取默认值
  getEEPROM();

  // 读取电源电压（毫伏）
  delay(100);
  Vcc = getVCC(); Vin = getVIN();

  // 读取并设置当前烙铁温度
  SetTemp  = DefaultTemp;
  RawTemp  = denoiseAnalog(SENSOR_PIN);
  ChipTemp = getChipTemp();
  calculateTemp();

  // 如果烙铁温度远低于设定点，则打开加热器
  if ((CurrentTemp + 20) < DefaultTemp) analogWrite(CONTROL_PIN, HEATER_ON);

  // 设置PID输出范围并启动PID
  ctrl.SetOutputLimits(0, 255);
  ctrl.SetMode(AUTOMATIC);

  // 设置初始旋转编码器值
  //a0 = PINB & 1; b0 = PIND >> 7 & 1; ab0 = (a0 == b0);
  a0 = 0;
  b0 = 0;
  setRotary(TEMP_MIN, TEMP_MAX, TEMP_STEP, DefaultTemp);

  // 重置休眠定时器
  sleepmillis = millis();

  lastLISTmp = getLISTemp();

  // 设置完成时长鸣提示
  beep(); beep();
}


void loop() {
  ROTARYCheck();      // 检查旋转编码器（温度/增强设置，进入设置菜单）
  SLEEPCheck();       // 检查并激活/停用休眠模式
  SENSORCheck();      // 读取烙铁的温度和振动开关
  Thermostat();       // 加热器控制
  MainScreen();       // 更新OLED上的主页面
}




// 检查旋转编码器；相应设置温度、切换增强模式、进入设置菜单
void ROTARYCheck() {
  // 根据旋转编码器值设置工作温度
  SetTemp = getRotary();

  // 检查旋转编码器开关
  uint8_t c = digitalRead(BUTTON_PIN);
  if ( !c && c0 ) {
    beep();
    buttonmillis = millis();
    while ( (!digitalRead(BUTTON_PIN)) && ((millis() - buttonmillis) < 500) );
    if ((millis() - buttonmillis) >= 500) SetupScreen();
    else {
      inBoostMode = !inBoostMode;
      if (inBoostMode) boostmillis = millis();
      handleMoved = true;
    }
  }
  c0 = c;

  // 检查增强模式下的定时器
  if (inBoostMode && timeOfBoost) {
    goneSeconds = (millis() - boostmillis) / 1000;
    if (goneSeconds >= timeOfBoost) {
      inBoostMode = false;              // 停止增强模式
      beep();                           // 增强模式结束时蜂鸣提示
      beepIfWorky = true;               // 达到工作温度时再次蜂鸣
    }
  }
}


// 检查并激活/停用休眠模式
void SLEEPCheck() {
  if (handleMoved) {                    // 如果手柄被移动
    if (inSleepMode) {                  // 是否在休眠或关机模式？
      if ((CurrentTemp + 20) < SetTemp) // 如果温度远低于设定点
        analogWrite(CONTROL_PIN, HEATER_ON);    // 立即启动加热器
      beep();                           // 唤醒时蜂鸣提示
      beepIfWorky = true;               // 达到工作温度时再次蜂鸣
    }
    handleMoved = false;                // 重置手柄移动标志
    inSleepMode = false;                // 重置休眠标志
    inOffMode   = false;                // 重置关机标志
    sleepmillis = millis();             // 重置休眠定时器
  }

  // 检查自手柄移动后经过的时间
  goneSeconds = (millis() - sleepmillis) / 1000;
  if ( (!inSleepMode) && (time2sleep > 0) && (goneSeconds >= time2sleep) ) {
    inSleepMode = true;
    beep();
  }
  if ( (!inOffMode)   && (time2off   > 0) && ((goneSeconds / 60) >= time2off  ) ) {
    inOffMode   = true;
    beep();
  }
}


// 读取温度、振动开关和电源电压
void SENSORCheck() {
  uint16_t sampleNum = 0;
  Accelero.Get_FIFO_Num_Samples(&sampleNum);
  if (sampleNum == 32) {
    int16_t accelerometer[3][32];
    for (int i = 0; i < 32; i++) {
      int32_t accelerometer_temp[3];
      Accelero.Get_X_Axes(accelerometer_temp);
      accelerometer[0][i] = accelerometer_temp[0];
      accelerometer[1][i] = accelerometer_temp[1];
      accelerometer[2][i] = accelerometer_temp[2];
    }
    if (variance(accelerometer[0]) > LISthreshold) {
      handleMoved = true;
    } else if (variance(accelerometer[1]) > LISthreshold) {
      handleMoved = true;
    } else if (variance(accelerometer[2]) > LISthreshold) {
      handleMoved = true;
    }
  }
  analogWrite(CONTROL_PIN, HEATER_OFF);       // 关闭加热器以便测量温度
  delayMicroseconds(TIME2SETTLE);             // 等待电压稳定

  double temp = denoiseAnalog(SENSOR_PIN);    // 读取温度的ADC值

  if (! SensorCounter--) Vin = getVIN();      // 定期获取输入电压

  analogWrite(CONTROL_PIN, HEATER_PWM);       // 重新打开加热器

  RawTemp += (temp - RawTemp) * SMOOTHIE;     // 稳定ADC温度读数
  calculateTemp();                            // 计算实际温度值

  // 当温度接近设定点时稳定显示的温度
  if ((ShowTemp != Setpoint) || (abs(ShowTemp - CurrentTemp) > 5)) ShowTemp = CurrentTemp;
  if (abs(ShowTemp - Setpoint) <= 1) ShowTemp = Setpoint;

  // 如果温度在工作范围内设置状态变量；如果刚达到工作温度则蜂鸣提示
  gap = abs(SetTemp - CurrentTemp);
  if (gap < 5) {
    if (!isWorky && beepIfWorky) beep();
    isWorky = true;
    beepIfWorky = false;
  }
  else isWorky = false;

  // 检查烙铁头是否存在或当前插入
  if (ShowTemp > 500) TipIsPresent = false;   // 烙铁头被移除？
  if (!TipIsPresent && (ShowTemp < 500)) {    // 新烙铁头插入？
    analogWrite(CONTROL_PIN, HEATER_OFF);     // 关闭加热器
    beep();                                   // 信息提示蜂鸣
    TipIsPresent = true;                      // 现在烙铁头存在
    ChangeTipScreen();                        // 显示烙铁头选择屏幕
    updateEEPROM();                           // 更新EEPROM中的设置
    handleMoved = true;                       // 重置所有定时器
    RawTemp  = denoiseAnalog(SENSOR_PIN);     // 重新启动温度平滑算法
    c0 = LOW;                                 // 开关必须释放
    setRotary(TEMP_MIN, TEMP_MAX, TEMP_STEP, SetTemp);  // 重置旋转编码器
  }
}


// 根据ADC读数和校准值计算实际温度值
void calculateTemp() {
  if      (RawTemp < 200) CurrentTemp = map (RawTemp,   0, 200,                     21, CalTemp[CurrentTip][0]);
  else if (RawTemp < 280) CurrentTemp = map (RawTemp, 200, 280, CalTemp[CurrentTip][0], CalTemp[CurrentTip][1]);
  else                    CurrentTemp = map (RawTemp, 280, 360, CalTemp[CurrentTip][1], CalTemp[CurrentTip][2]);
}


// 控制加热器
void Thermostat() {
  // 根据当前工作模式定义设定点
  if      (inOffMode)   Setpoint = 0;
  else if (inSleepMode) Setpoint = SleepTemp;
  else if (inBoostMode) Setpoint = SetTemp + BoostTemp;
  else                  Setpoint = SetTemp;

  // 控制加热器（PID或直接控制）
  gap = abs(Setpoint - CurrentTemp);
  if (PIDenable) {
    Input = CurrentTemp;
    if (gap < 30) ctrl.SetTunings(consKp, consKi, consKd);
    else ctrl.SetTunings(aggKp, aggKi, aggKd);
    ctrl.Compute();
  } else {
    // 如果当前温度低于设定点则打开加热器
    if ((CurrentTemp + 0.5) < Setpoint) Output = 0; else Output = 255;
  }
  analogWrite(CONTROL_PIN, HEATER_PWM);     // 设置加热器PWM
}


// 在蜂鸣器上创建短促的蜂鸣声
void beep() {
  if (beepEnable) {
    for (uint8_t i = 0; i < 255; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delayMicroseconds(125);
      digitalWrite(BUZZER_PIN, LOW);
      delayMicroseconds(125);
    }
  }
}


// 设置旋转编码器的起始值
void setRotary(int rmin, int rmax, int rstep, int rvalue) {
  countMin  = rmin << ROTARY_TYPE;
  countMax  = rmax << ROTARY_TYPE;
  countStep = rstep;
  count     = rvalue << ROTARY_TYPE;
}


// 读取当前旋转编码器值
int getRotary() {
  Button_loop();
  return (count >> ROTARY_TYPE);
}


// 从EEPROM读取用户设置；如果EEPROM值无效，则写入默认值
void getEEPROM() {
  uint16_t identifier = (EEPROM.read(0) << 8) | EEPROM.read(1);
  if (identifier == EEPROM_IDENT) {
    DefaultTemp = (EEPROM.read(2) << 8) | EEPROM.read(3);
    SleepTemp   = (EEPROM.read(4) << 8) | EEPROM.read(5);
    BoostTemp   =  EEPROM.read(6);
    time2sleep  =  EEPROM.read(7);
    time2off    =  EEPROM.read(8);
    timeOfBoost =  EEPROM.read(9);
    MainScrType =  EEPROM.read(10);
    PIDenable   =  EEPROM.read(11);
    beepEnable  =  EEPROM.read(12);
    CurrentTip  =  EEPROM.read(13);
    NumberOfTips = EEPROM.read(14);
    LISthreshold = EEPROM.read(15);


    uint8_t i, j;
    uint16_t counter = 16;
    for (i = 0; i < NumberOfTips; i++) {
      for (j = 0; j < TIPNAMELENGTH; j++) {
        TipName[i][j] = EEPROM.read(counter++);
      }
      for (j = 0; j < 4; j++) {
        CalTemp[i][j]  = EEPROM.read(counter++) << 8;
        CalTemp[i][j] |= EEPROM.read(counter++);
      }
    }
  }
  else {
    EEPROM.update(0, EEPROM_IDENT >> 8); EEPROM.update(1, EEPROM_IDENT & 0xFF);
    updateEEPROM();
  }
}


// 使用更新功能将用户设置写入EEPROM以最小化写入周期
void updateEEPROM() {
  EEPROM.update( 2, DefaultTemp >> 8);
  EEPROM.update( 3, DefaultTemp & 0xFF);
  EEPROM.update( 4, SleepTemp >> 8);
  EEPROM.update( 5, SleepTemp & 0xFF);
  EEPROM.update( 6, BoostTemp);
  EEPROM.update( 7, time2sleep);
  EEPROM.update( 8, time2off);
  EEPROM.update( 9, timeOfBoost);
  EEPROM.update(10, MainScrType);
  EEPROM.update(11, PIDenable);
  EEPROM.update(12, beepEnable);
  EEPROM.update(13, CurrentTip);
  EEPROM.update(14, NumberOfTips);
  EEPROM.update(15, LISthreshold);

  uint8_t i, j;
  uint16_t counter = 16;
  for (i = 0; i < NumberOfTips; i++) {
    for (j = 0; j < TIPNAMELENGTH; j++) EEPROM.update(counter++, TipName[i][j]);
    for (j = 0; j < 4; j++) {
      EEPROM.update(counter++, CalTemp[i][j] >> 8);
      EEPROM.update(counter++, CalTemp[i][j] & 0xFF);
    }
  }
}


// 绘制主屏幕
void MainScreen() {
  u8g.firstPage();
  do {
    // 绘制设定点温度
    u8g.setFont(u8g2_font_9x15_tf);
    u8g.setFontPosTop();
    u8g.drawStr( 0, 0,  "SET:");
    u8g.setCursor(40, 0);
    u8g.print(Setpoint, 0);
    //    int32_t accelerometer[3];
    //    Accelero.Get_X_Axes(accelerometer);
    //    u8g.setCursor(0, 0);
    //    u8g.print(accelerometer[0], DEC);

    // 绘制加热器状态
    u8g.setCursor(83, 0);
    if (ShowTemp > 500)    u8g.print(F("ERROR"));
    else if (inOffMode)    u8g.print(F("  OFF"));
    else if (inSleepMode)  u8g.print(F("SLEEP"));
    else if (inBoostMode)  u8g.print(F("BOOST"));
    else if (isWorky)      u8g.print(F("WORKY"));
    else if (Output < 180) u8g.print(F(" HEAT"));
    else                   u8g.print(F(" HOLD"));

    // 其余部分取决于主屏幕类型
    if (MainScrType) {
      // 绘制当前烙铁头和输入电压
      float fVin = (float)Vin / 1000;     // 将毫伏转换为伏特
      newLISTmp = newLISTmp + 0.01 * getLISTemp();
      LISTmpTime++;
      if (LISTmpTime >= 100) {
        lastLISTmp = newLISTmp;
        newLISTmp = 0;
        LISTmpTime = 0;
      }
      u8g.setCursor( 0, 52); u8g.print(lastLISTmp, 1); u8g.print(F("C"));
      u8g.setCursor(83, 52); u8g.print(fVin, 1); u8g.print(F("V"));
      // 绘制当前温度
      u8g.setFont(u8g2_font_freedoomr25_tn);
      u8g.setFontPosTop();
      u8g.setCursor(37, 22);
      if (ShowTemp > 500) u8g.print(F("000")); else u8g.print(ShowTemp);
    } else {
      // 以大数字绘制当前温度
      u8g.setFont(u8g2_font_fub42_tn);
      u8g.setFontPosTop();
      u8g.setCursor(15, 20);
      if (ShowTemp > 500) u8g.print(F("000")); else u8g.print(ShowTemp);
    }
  } while (u8g.nextPage());
}


// 设置屏幕
void SetupScreen() {
  analogWrite(CONTROL_PIN, HEATER_OFF);      // 关闭加热器
  beep();
  uint16_t SaveSetTemp = SetTemp;
  uint8_t selection = 0;
  bool repeat = true;

  while (repeat) {
    selection = MenuScreen(SetupItems, sizeof(SetupItems), selection);
    switch (selection) {
      case 0:   TipScreen(); repeat = false; break;
      case 1:   TempScreen(); break;
      case 2:   TimerScreen(); break;
      case 3:   PIDenable = MenuScreen(ControlTypeItems, sizeof(ControlTypeItems), PIDenable); break;
      case 4:   MainScrType = MenuScreen(MainScreenItems, sizeof(MainScreenItems), MainScrType); break;
      case 5:   beepEnable = MenuScreen(BuzzerItems, sizeof(BuzzerItems), beepEnable); break;
      case 6:   InfoScreen(); break;
      default:  repeat = false; break;
    }
  }
  updateEEPROM();
  handleMoved = true;
  SetTemp = SaveSetTemp;
  setRotary(TEMP_MIN, TEMP_MAX, TEMP_STEP, SetTemp);
}


// 烙铁头设置屏幕
void TipScreen() {
  uint8_t selection = 0;
  bool repeat = true;
  while (repeat) {
    selection = MenuScreen(TipItems, sizeof(TipItems), selection);
    switch (selection) {
      case 0:   ChangeTipScreen();   break;
      case 1:   CalibrationScreen(); break;
      case 2:   InputNameScreen();   break;
      case 3:   DeleteTipScreen();   break;
      case 4:   AddTipScreen();      break;
      default:  repeat = false;      break;
    }
  }
}


// 温度设置屏幕
void TempScreen() {
  uint8_t selection = 0;
  bool repeat = true;
  while (repeat) {
    selection = MenuScreen(TempItems, sizeof(TempItems), selection);
    switch (selection) {
      case 0:   setRotary(TEMP_MIN, TEMP_MAX, TEMP_STEP, DefaultTemp);
        DefaultTemp = InputScreen(DefaultTempItems); break;
      case 1:   setRotary(20, 200, TEMP_STEP, SleepTemp);
        SleepTemp = InputScreen(SleepTempItems); break;
      case 2:   setRotary(10, 100, TEMP_STEP, BoostTemp);
        BoostTemp = InputScreen(BoostTempItems); break;
      default:  repeat = false; break;
    }
  }
}


// 定时器设置屏幕
void TimerScreen() {
  uint8_t selection = 0;
  bool repeat = true;
  while (repeat) {
    selection = MenuScreen(TimerItems, sizeof(TimerItems), selection);
    switch (selection) {
      case 0:   setRotary(0, 600, 10, time2sleep);
        time2sleep = InputScreen(SleepTimerItems); break;
      case 1:   setRotary(0, 60, 5, time2off);
        time2off = InputScreen(OffTimerItems); break;
case 2:   setRotary(0, 180, 10, timeOfBoost);
        timeOfBoost = InputScreen(BoostTimerItems); break;
      case 3:   setRotary(0, 50, 1, LISthreshold);
        LISthreshold = InputScreen(LISthresholdItems); break;
      default:  repeat = false; break;
    }
  }
}


// 菜单屏幕 - 显示选项列表让用户选择
uint8_t MenuScreen(const char *Items[], uint8_t numberOfItems, uint8_t selected) {
  bool isTipScreen = (Items[0] == "Tip:");
  uint8_t lastselected = selected;
  int8_t  arrow = 0;
  if (selected) arrow = 1;
  numberOfItems >>= 1;
  setRotary(0, numberOfItems - 2, 1, selected);
  bool    lastbutton = (!digitalRead(BUTTON_PIN));

  do {
    selected = getRotary();
    arrow = constrain(arrow + selected - lastselected, 0, 2);
    lastselected = selected;
    u8g.firstPage();
    do {
      u8g.setFont(u8g2_font_9x15_tf);
      u8g.setFontPosTop();
      u8g.drawStr( 0, 0,  Items[0]);
      if (isTipScreen) u8g.drawStr( 54, 0,  TipName[CurrentTip]);
      u8g.drawStr( 0, 16 * (arrow + 1), ">");
      for (uint8_t i = 0; i < 3; i++) {
        uint8_t drawnumber = selected + i + 1 - arrow;
        if (drawnumber < numberOfItems)
          u8g.drawStr( 12, 16 * (i + 1), Items[selected + i + 1 - arrow]);
      }
    } while (u8g.nextPage());
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
  return selected;
}


// 消息显示屏幕 - 显示多条消息信息
void MessageScreen(const char *Items[], uint8_t numberOfItems) {
  bool lastbutton = (!digitalRead(BUTTON_PIN));
  u8g.firstPage();
  do {
    u8g.setFont(u8g2_font_9x15_tf);
    u8g.setFontPosTop();
    for (uint8_t i = 0; i < numberOfItems; i++) u8g.drawStr( 0, i * 16,  Items[i]);
  } while (u8g.nextPage());
  do {
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);
  beep();
}


// 输入数值屏幕 - 让用户通过旋转编码器输入数值
uint16_t InputScreen(const char *Items[]) {
  uint16_t  value;
  bool      lastbutton = (!digitalRead(BUTTON_PIN));

  do {
    value = getRotary();
    u8g.firstPage();
    do {
      u8g.setFont(u8g2_font_9x15_tf);
      u8g.setFontPosTop();
      u8g.drawStr( 0, 0,  Items[0]);
      u8g.setCursor(0, 32); u8g.print(">"); u8g.setCursor(10, 32);
      if (value == 0)  u8g.print(F("Deactivated"));
      else            {
        u8g.print(value);
        u8g.print(" ");
        u8g.print(Items[1]);
      }
    } while (u8g.nextPage());
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
  return value;
}


// 信息显示屏幕 - 显示系统各种状态信息
void InfoScreen() {
  bool lastbutton = (!digitalRead(BUTTON_PIN));

  do {
    Vcc = getVCC();                     // 读取输入电压（单位：毫伏）
    float fVcc = (float)Vcc / 1000;     // 将毫伏转换为伏特
    Vin = getVIN();                     // 读取供电电压（单位：毫伏）
    float fVin = (float)Vin / 1000;     // 将毫伏转换为伏特
    float fTmp = getChipTemp();         // 读取芯片内部温度
    int32_t accelerometer[3];
    Accelero.Get_X_Axes(accelerometer);
    u8g.firstPage();
    do {
      u8g.setFont(u8g2_font_9x15_tf);
      u8g.setFontPosTop();
      u8g.setCursor(0,  0); u8g.print(F("温度: "));  u8g.print(fTmp, 1); u8g.print(F(" 度"));
      u8g.setCursor(0, 16); u8g.print(F("输入: "));  u8g.print(fVin, 1); u8g.print(F(" 伏"));
      u8g.setCursor(0, 32); u8g.print(F("Vcc:  ")); u8g.print(fVcc, 1); u8g.print(F(" 伏"));
      u8g.setCursor(0, 48); u8g.print(F("IMU:  ")); u8g.print(accelerometer[1], DEC); u8g.print(F(""));
    } while (u8g.nextPage());
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
}


// 更换烙铁头屏幕 - 选择要使用的烙铁头
void ChangeTipScreen() {
  uint8_t selected = CurrentTip;
  uint8_t lastselected = selected;
  int8_t  arrow = 0;
  if (selected) arrow = 1;
  setRotary(0, NumberOfTips - 1, 1, selected);
  bool    lastbutton = (!digitalRead(BUTTON_PIN));

  do {
    selected = getRotary();
    arrow = constrain(arrow + selected - lastselected, 0, 2);
    lastselected = selected;
    u8g.firstPage();
    do {
      u8g.setFont(u8g2_font_9x15_tf);
      u8g.setFontPosTop();
      strcpy_P(F_Buffer, PSTR("Select Tip"));
      u8g.drawStr( 0, 0,  F_Buffer);
      u8g.drawStr( 0, 16 * (arrow + 1), ">");
      for (uint8_t i = 0; i < 3; i++) {
        uint8_t drawnumber = selected + i - arrow;
        if (drawnumber < NumberOfTips)
          u8g.drawStr( 12, 16 * (i + 1), TipName[selected + i - arrow]);
      }
    } while (u8g.nextPage());
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
  CurrentTip = selected;
}


// 温度校准屏幕 - 校准烙铁头的温度测量
void CalibrationScreen() {
  uint16_t CalTempNew[4];
  for (uint8_t CalStep = 0; CalStep < 3; CalStep++) {
    SetTemp = CalTemp[CurrentTip][CalStep];
    setRotary(100, 500, 1, SetTemp);
    beepIfWorky = true;
    bool    lastbutton = (!digitalRead(BUTTON_PIN));

    do {
      SENSORCheck();      // 读取烙铁的温度和振动开关状态
      Thermostat();       // 控制加热器

      u8g.firstPage();
      do {
        u8g.setFont(u8g2_font_9x15_tf);
        u8g.setFontPosTop();
        strcpy_P(F_Buffer, PSTR("温度校准"));
        u8g.drawStr( 0, 0, F_Buffer);
        u8g.setCursor(0, 16); u8g.print(F("步骤: ")); u8g.print(CalStep + 1); u8g.print(" / 3");
        if (isWorky) {
          u8g.setCursor(0, 32); u8g.print(F("设置测量到的"));
          u8g.setCursor(0, 48); u8g.print(F("温度: ")); u8g.print(getRotary());
        } else {
          u8g.setCursor(0, 32); u8g.print(F("ADC值: ")); u8g.print(uint16_t(RawTemp));
          u8g.setCursor(0, 48); u8g.print(F("请等待..."));
        }
      } while (u8g.nextPage());
      if (lastbutton && digitalRead(BUTTON_PIN)) {
        delay(10);
        lastbutton = false;
      }
    } while (digitalRead(BUTTON_PIN) || lastbutton);

    CalTempNew[CalStep] = getRotary();
    beep(); delay (10);
  }

  analogWrite(CONTROL_PIN, HEATER_OFF);       // 关闭加热器
  delayMicroseconds(TIME2SETTLE);             // 等待电压稳定
  CalTempNew[3] = getChipTemp();              // 读取芯片温度
  if ((CalTempNew[0] + 10 < CalTempNew[1]) && (CalTempNew[1] + 10 < CalTempNew[2])) {
    if (MenuScreen(StoreItems, sizeof(StoreItems), 0)) {
      for (uint8_t i = 0; i < 4; i++) CalTemp[CurrentTip][i] = CalTempNew[i];
    }
  }
}


// 输入烙铁头名称屏幕 - 为烙铁头设置名称
void InputNameScreen() {
  uint8_t  value;

  for (uint8_t digit = 0; digit < (TIPNAMELENGTH - 1); digit++) {
    bool      lastbutton = (!digitalRead(BUTTON_PIN));
    setRotary(31, 96, 1, 65);
    do {
      value = getRotary();
      if (value == 31) {
        value = 95;
        setRotary(31, 96, 1, 95);
      }
      if (value == 96) {
        value = 32;
        setRotary(31, 96, 1, 32);
      }
      u8g.firstPage();
      do {
        u8g.setFont(u8g2_font_9x15_tf);
        u8g.setFontPosTop();
        strcpy_P(F_Buffer, PSTR("Enter Tip Name"));
        u8g.drawStr( 0, 0,  F_Buffer);
        u8g.setCursor(9 * digit, 48); u8g.print(char(94));
        u8g.setCursor(0, 32);
        for (uint8_t i = 0; i < digit; i++) u8g.print(TipName[CurrentTip][i]);
        u8g.setCursor(9 * digit, 32); u8g.print(char(value));
      } while (u8g.nextPage());
      if (lastbutton && digitalRead(BUTTON_PIN)) {
        delay(10);
        lastbutton = false;
      }
    } while (digitalRead(BUTTON_PIN) || lastbutton);
    TipName[CurrentTip][digit] = value;
    beep(); delay (10);
  }
  TipName[CurrentTip][TIPNAMELENGTH - 1] = 0;
  return;
}


// delete tip screen
void DeleteTipScreen() {
  if (NumberOfTips == 1) {
    MessageScreen(DeleteMessage, sizeof(DeleteMessage));
  }
  else if (MenuScreen(SureItems, sizeof(SureItems), 0)) {
    if (CurrentTip == (NumberOfTips - 1)) {
      CurrentTip--;
    }
    else {
      for (uint8_t i = CurrentTip; i < (NumberOfTips - 1); i++) {
        for (uint8_t j = 0; j < TIPNAMELENGTH; j++) TipName[i][j] = TipName[i + 1][j];
        for (uint8_t j = 0; j < 4; j++)             CalTemp[i][j] = CalTemp[i + 1][j];
      }
    }
    NumberOfTips--;
  }
}


// add new tip screen
void AddTipScreen() {
  if (NumberOfTips < TIPMAX) {
    CurrentTip = NumberOfTips++; InputNameScreen();
    CalTemp[CurrentTip][0] = TEMP200; CalTemp[CurrentTip][1] = TEMP280;
    CalTemp[CurrentTip][2] = TEMP360; CalTemp[CurrentTip][3] = TEMPCHP;
  } else MessageScreen(MaxTipMessage, sizeof(MaxTipMessage));
}


// average several ADC readings in sleep mode to denoise
uint16_t denoiseAnalog (byte port) {
  uint16_t result = 0;
  ADCSRA |= bit (ADEN) | bit (ADIF);    // enable ADC, turn off any pending interrupt
  if (port >= A0) port -= A0;           // set port and
  ADMUX = (0x0F & port) | bit(REFS0);   // reference to AVcc
  set_sleep_mode (SLEEP_MODE_ADC);      // sleep during sample for noise reduction
  for (uint8_t i = 0; i < 32; i++) {    // get 32 readings
    sleep_mode();                       // go to sleep while taking ADC sample
    while (bitRead(ADCSRA, ADSC));      // make sure sampling is completed
    result += ADC;                      // add them up
  }
  bitClear (ADCSRA, ADEN);              // disable ADC
  return (result >> 5);                 // devide by 32 and return value
}


// get internal temperature by reading ADC channel 8 against 1.1V reference
double getChipTemp() {
  uint16_t result = 0;
  ADCSRA |= bit (ADEN) | bit (ADIF);    // enable ADC, turn off any pending interrupt
  ADMUX = bit (REFS1) | bit (REFS0) | bit (MUX3); // set reference and mux
  delay(20);                            // wait for voltages to settle
  set_sleep_mode (SLEEP_MODE_ADC);      // sleep during sample for noise reduction
  for (uint8_t i = 0; i < 32; i++) {    // get 32 readings
    sleep_mode();                       // go to sleep while taking ADC sample
    while (bitRead(ADCSRA, ADSC));      // make sure sampling is completed
    result += ADC;                      // add them up
  }
  bitClear (ADCSRA, ADEN);              // disable ADC
  result >>= 2;                         // devide by 4
  return ((result - 2594) / 9.76);      // calculate internal temperature in degrees C
}


// get LIS temperature
float getLISTemp() {
  uint8_t a, b;
  uint32_t i = 0;
  Accelero.ReadReg(0x0D, &a);
  Accelero.ReadReg(0x0E, &b);
  i = b;
  return (float)((i << 4) | (a >> 4)) / 16.0 + 25.0;      // calculate LIS temperature in degrees C
}


// get input voltage in mV by reading 1.1V reference against AVcc
uint16_t getVCC() {
  uint16_t result = 0;
  ADCSRA |= bit (ADEN) | bit (ADIF);    // enable ADC, turn off any pending interrupt
  // set Vcc measurement against 1.1V reference
  ADMUX = bit (REFS0) | bit (MUX3) | bit (MUX2) | bit (MUX1);
  delay(1);                             // wait for voltages to settle
  set_sleep_mode (SLEEP_MODE_ADC);      // sleep during sample for noise reduction
  for (uint8_t i = 0; i < 16; i++) {    // get 16 readings
    sleep_mode();                       // go to sleep while taking ADC sample
    while (bitRead(ADCSRA, ADSC));      // make sure sampling is completed
    result += ADC;                      // add them up
  }
  bitClear (ADCSRA, ADEN);              // disable ADC
  result >>= 4;                         // devide by 16
  return (1125300L / result);           // 1125300 = 1.1 * 1023 * 1000
}


// get supply voltage in mV
uint16_t getVIN() {
  long result;
  result = denoiseAnalog (VIN_PIN);     // read supply voltage via voltage divider
  return (result * Vcc / 179.474);      // 179.474 = 1023 * R13 / (R12 + R13)
}

int32_t variance(int16_t a[])
{
  // Compute mean (average of elements)
  int32_t sum = 0;

  for (int i = 0; i < 32; i++) sum += a[i];
  int16_t mean = (int32_t)sum / 32;
  // Compute sum squared differences with mean.
  int32_t sqDiff = 0;
  for (int i = 0; i < 32; i++)
    sqDiff += (a[i] - mean) * (a[i] - mean);
  return (int32_t)sqDiff / 32;
}


// ADC interrupt service routine
EMPTY_INTERRUPT (ADC_vect);             // nothing to be done here


// // Pin change interrupt service routine for rotary encoder
// ISR (PCINT0_vect) {
//   uint8_t a = PINB & 1;
//   uint8_t b = PIND >> 7 & 1;
//
//   if (a != a0) {              // A changed
//     a0 = a;
//     if (b != b0) {            // B changed
//       b0 = b;
//       count = constrain(count + ((a == b) ? countStep : -countStep), countMin, countMax);
//       if (ROTARY_TYPE && ((a == b) != ab0)) {
//         count = constrain(count + ((a == b) ? countStep : -countStep), countMin, countMax);;
//       }
//       ab0 = (a == b);
//       handleMoved = true;
//     }
//   }
// }

// // Button2 handler
// void Pressed_N(Button2& btn) {
//   count = constrain(count - countStep, countMin, countMax);
// }
//
// void Pressed_P(Button2& btn) {
//   count = constrain(count + countStep, countMin, countMax);
// }

void Button_loop() {
  if (!digitalRead(BUTTON_N_PIN) && a0 == 1) {
    delay(BUTTON_DELAY);
    if (!digitalRead(BUTTON_N_PIN)) {
      count = constrain(count - countStep, countMin, countMax);
      a0 = 0;
    }
  } else if (digitalRead(BUTTON_N_PIN)) {
    a0 = 1;
  }

  if (!digitalRead(BUTTON_P_PIN) && b0 == 1) {
    delay(BUTTON_DELAY);
    if (!digitalRead(BUTTON_P_PIN)) {
      count = constrain(count + countStep, countMin, countMax);
      b0 = 0;
    }
  } else if (digitalRead(BUTTON_P_PIN)) {
    b0 = 1;
  }
}