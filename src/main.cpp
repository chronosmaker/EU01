#include <SPI.h>
#include <WiFi.h>
#include "Common.h"
#include "DW1000Ranging.h"
#include "DW1000.h"

// ========== 配置区域 ==========
// 取消注释下一行来编译为TAG，注释掉则编译为ANCHOR
// #define TAG_MODE

// 设备地址配置
#ifdef TAG_MODE
char TAG_ADDR[] = "7D:00:22:EA:82:60:3B:9C";  // TAG设备地址
#else
char ANCHOR_ADDR[] = "83:17:5B:B5:D9:00:00:01";  // ANCHOR设备地址
#endif

// 天线延迟校准值（单位：皮秒）
uint16_t antennaDelay = 16390;  // 默认值，需要实际校准

// 测距模式配置
const byte RANGING_MODE[] = {
  DW1000.TRX_RATE_6800KBPS,     // 数据速率
  DW1000.TX_PULSE_FREQ_64MHZ,   // 脉冲频率
  DW1000.TX_PREAMBLE_LEN_128    // 前导码长度
};

// ========== 全局变量 ==========
#ifdef TAG_MODE
  // TAG专用变量
float currentRange = 0.0;
float currentPower = 0.0;
#else
  // ANCHOR专用变量
#endif

// ========== 函数声明 ==========
void calibrateAntennaDelay(float actualDistance);
void performCalibration();
void printDeviceInfo();
void handleCommand(char command);
void displayRangeInfo();

// ========== 初始化函数 ==========
void setup() {
  setCpuFrequencyMhz(80);
  Serial.print("CPU频率: ");
  Serial.println(getCpuFrequencyMhz());

  // // 2. 禁用无线功能
  // WiFi.mode(WIFI_OFF);
  // btStop();
  // Serial.println("无线功能已禁用");

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== DW1000 UWB测距系统 ===");

  // 初始化SPI
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, UWB_CS);

  // 初始化DW1000
  DW1000Ranging.initCommunication(UWB_RST, UWB_CS, UWB_IRQ);

  // 打印设备信息
  printDeviceInfo();

  // 启动设备
#ifdef TAG_MODE
  Serial.println("设备模式: TAG");
  DW1000Ranging.startAsTag(TAG_ADDR, RANGING_MODE, false);

  // 注册测距回调
  DW1000Ranging.attachNewRange([]() {
    Serial.println("收到新的测距数据:");
    DW1000Device* device = DW1000Ranging.getDistantDevice();
    if (device != nullptr) {
      currentRange = device->getRange();
      currentPower = device->getRXPower();

      Serial.print("距离: ");
      Serial.print(currentRange);
      Serial.println(" m");

      Serial.print("信号强度: ");
      Serial.print(currentPower);
      Serial.println(" dBm");

      Serial.print("首发路径功率: ");
      Serial.print(device->getFPPower());
      Serial.println(" dBm");

      Serial.print("信号质量: ");
      Serial.print(device->getQuality());
      Serial.println();
      Serial.println("-------------------");
    }
    });

#else
  Serial.println("设备模式: ANCHOR");
  DW1000Ranging.startAsAnchor(ANCHOR_ADDR, RANGING_MODE, false);

  // 注册新设备发现回调
  DW1000Ranging.attachNewDevice([](DW1000Device* device) {
    Serial.print("发现新设备: ");
    Serial.println(device->getShortAddress(), HEX);
    });

  // 注册测距完成回调
  DW1000Ranging.attachNewRange([]() {
    DW1000Device* device = DW1000Ranging.getDistantDevice();
    if (device != nullptr) {
      float dist = device->getRange();
      if (dist > 0 && dist < 100) {
        Serial.print("与设备 ");
        Serial.print(device->getShortAddress(), HEX);
        Serial.print(" 的测距完成: ");
        Serial.print(dist);
        Serial.println(" m");
      }
    }
    });

#endif

  // 设置天线延迟
  DW1000.setAntennaDelay(antennaDelay);
  Serial.print("设置天线延迟: ");
  Serial.println(antennaDelay);

  Serial.println("系统初始化完成!");
  Serial.println("输入命令:");
  Serial.println("  'c' - 执行天线延迟校准");
  Serial.println("  'r' - 显示当前测距信息");
  Serial.println("  'i' - 显示设备信息");
  Serial.println("  'a' - 显示天线延迟");
}

// ========== 主循环 ==========
void loop() {
  // 处理DW1000事件
  DW1000Ranging.loop();

  // 处理串口命令
  if (Serial.available()) {
    char command = Serial.read();
    handleCommand(command);
  }

  // TAG模式下的定期测距显示
#ifdef TAG_MODE
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay > 2000) {  // 每2秒显示一次
    lastDisplay = millis();
    if (currentRange > 0) {
      Serial.print("[实时] 距离: ");
      Serial.print(currentRange, 3);
      Serial.print(" m, 信号: ");
      Serial.print(currentPower, 1);
      Serial.println(" dBm");
    }
  }
#endif
}

// ========== 命令处理函数 ==========
void handleCommand(char command) {
  switch (command) {
  case 'c':  // 校准
    performCalibration();
    break;

  case 'r':  // 显示测距信息
    displayRangeInfo();
    break;

  case 'i':  // 显示设备信息
    printDeviceInfo();
    break;

  case 'a':  // 显示天线延迟
    Serial.print("当前天线延迟: ");
    Serial.println(DW1000.getAntennaDelay());
    break;

  case '\n':  // 忽略换行符
  case '\r':
    break;

  default:
    Serial.println("未知命令，可用命令: c, r, i, a");
    break;
  }
}

// ========== 设备信息打印 ==========
void printDeviceInfo() {
  char msg[128];

  Serial.println("\n=== 设备信息 ===");

  // 设备标识符
  DW1000.getPrintableDeviceIdentifier(msg);
  Serial.print("设备标识: ");
  Serial.println(msg);

  // 扩展唯一标识符
  DW1000.getPrintableExtendedUniqueIdentifier(msg);
  Serial.print("EUI: ");
  Serial.println(msg);

  // 网络ID和短地址
  DW1000.getPrintableNetworkIdAndShortAddress(msg);
  Serial.print("网络地址: ");
  Serial.println(msg);

  // 设备模式
  DW1000.getPrintableDeviceMode(msg);
  Serial.print("工作模式: ");
  Serial.println(msg);

  // 天线延迟
  Serial.print("天线延迟: ");
  Serial.println(DW1000.getAntennaDelay());

  Serial.println("================\n");
}

// ========== 测距信息显示 ==========
void displayRangeInfo() {
#ifdef TAG_MODE
  Serial.println("\n=== TAG测距信息 ===");
  Serial.print("当前距离: ");
  Serial.print(currentRange, 3);
  Serial.println(" m");

  Serial.print("信号强度: ");
  Serial.print(currentPower, 1);
  Serial.println(" dBm");

  // 显示所有已发现的ANCHOR设备
  uint8_t deviceCount = DW1000Ranging.getNetworkDevicesNumber();
  Serial.print("发现的ANCHOR数量: ");
  Serial.println(deviceCount);

  for (int i = 0; i < deviceCount; i++) {
    // 注意：这里需要根据实际库结构调整
    Serial.print("  Anchor ");
    Serial.println(i);
  }

#else
  Serial.println("\n=== ANCHOR信息 ===");
  uint8_t deviceCount = DW1000Ranging.getNetworkDevicesNumber();
  Serial.print("连接的TAG数量: ");
  Serial.println(deviceCount);

  // 这里可以添加更多ANCHOR特定的信息显示
  Serial.print("天线延迟: ");
  Serial.println(DW1000.getAntennaDelay());

#endif
  Serial.println("================\n");
}

// ========== 校准功能 ==========
void performCalibration() {
  Serial.println("\n=== 开始天线延迟校准 ===");
  Serial.println("注意: 校准需要在已知精确距离的环境下进行!");
  Serial.println("请将TAG和ANCHOR放置在已知距离的位置");
  Serial.println("然后输入实际距离（单位：米）");
  Serial.println("或输入 '0' 取消校准");

  // 等待用户输入实际距离
  while (!Serial.available()) {
    delay(100);
  }

  String input = Serial.readString();
  input.trim();
  float actualDistance = input.toFloat();

  if (actualDistance <= 0) {
    Serial.println("校准取消");
    return;
  }

  Serial.print("输入的实际距离: ");
  Serial.print(actualDistance);
  Serial.println(" m");

  calibrateAntennaDelay(actualDistance);
}

void calibrateAntennaDelay(float actualDistance) {
  Serial.println("开始校准过程...");

#ifdef TAG_MODE
  Serial.println("错误: 校准需要在ANCHOR模式下进行!");
  Serial.println("请将设备切换为ANCHOR模式后重试");
  return;
#else
  // ANCHOR模式下的校准逻辑
  // 这里需要多次测量并计算平均值
  const int numMeasurements = 10;
  float measuredDistances[numMeasurements];
  float sum = 0.0;

  Serial.println("正在进行测量...");

  for (int i = 0; i < numMeasurements; i++) {
    // 等待新的测距数据
    unsigned long startTime = millis();
    while (DW1000Ranging.getNetworkDevicesNumber() == 0 &&
      (millis() - startTime) < 5000) {
      DW1000Ranging.loop();
      delay(100);
    }

    if (DW1000Ranging.getNetworkDevicesNumber() > 0) {
      DW1000Device* device = DW1000Ranging.getDistantDevice();
      if (device != nullptr) {
        measuredDistances[i] = device->getRange();
        sum += measuredDistances[i];
        Serial.print("测量 ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(measuredDistances[i], 3);
        Serial.println(" m");
      }
    }

    delay(1000);
  }

  float averageMeasured = sum / numMeasurements;
  Serial.print("平均测量距离: ");
  Serial.print(averageMeasured, 3);
  Serial.println(" m");

  // 计算误差并调整天线延迟
  float error = averageMeasured - actualDistance;
  Serial.print("测量误差: ");
  Serial.print(error, 3);
  Serial.println(" m");

  // 简单的校准算法：根据误差调整天线延迟
  // 注意：这只是一个示例，实际校准算法可能更复杂
  uint16_t currentDelay = DW1000.getAntennaDelay();
  uint16_t newDelay = currentDelay;

  if (abs(error) > 0.01) {  // 如果误差大于1cm
    // 根据误差方向调整延迟（需要根据实际硬件特性调整系数）
    int16_t adjustment = (int16_t)(error * 10000);  // 简化调整
    newDelay = currentDelay + adjustment;

    // 限制在合理范围内
    if (newDelay < 10000) newDelay = 10000;
    if (newDelay > 30000) newDelay = 30000;

    // 应用新的天线延迟
    DW1000.setAntennaDelay(newDelay);
    antennaDelay = newDelay;

    Serial.print("天线延迟从 ");
    Serial.print(currentDelay);
    Serial.print(" 调整为 ");
    Serial.println(newDelay);
  } else {
    Serial.println("误差很小，无需调整");
  }

  Serial.println("校准完成!");

#endif
}