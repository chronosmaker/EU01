#include "SSD1327_GFX.h"

// ESP32-S3 SPI引脚定义
#define OLED_CLK    7   // SCK
#define OLED_MOSI   9   // MOSI  
#define OLED_CS     6   // 片选
#define OLED_DC     3   // 数据/命令
#define OLED_RESET  4   // 复位

// 创建SSD1327显示对象
SSD1327_GFX display(96, 96, &SPI, OLED_DC, OLED_RESET, OLED_CS, 8000000);

void setup() {
  Serial.begin(115200);

  // 初始化显示
  if (!display.begin()) {
    Serial.println("SSD1327初始化失败!");
    while (1);
  }

  Serial.println("SSD1327 GFX示例启动");

  // 测试Adafruit_GFX功能

  // 1. 绘制文本
  // display.setTextSize(1);
  // display.setTextColor(MONOOLED_WHITE);
  // display.setCursor(0, 0);
  // display.println("Hello GFX!");

  // 2. 绘制基本图形
  // display.drawRect(0, 20, 30, 20, MONOOLED_WHITE);  // 矩形
  // display.fillRect(35, 20, 30, 20, 8);              // 填充矩形，中等灰度
  // display.drawCircle(50, 60, 15, MONOOLED_WHITE);   // 圆形
  // display.fillCircle(80, 60, 10, 15);               // 填充圆形，高灰度

  // 3. 绘制线条
  // for (int i = 0; i < 16; i++) {
  //   display.drawLine(0, 80 + i, 20 + i * 5, 80 + i, i);  // 不同灰度的线条
  // }

  // 4. 绘制渐变
  // for (int x = 0; x < 96; x++) {
  //   uint8_t gray = (x * 16) / 96;  // 水平渐变
  //   display.drawFastVLine(x, 45, 15, gray);
  // }

  // 更新显示
  display.display();
}

void loop() {
  // 动态效果：移动的方块
  static int x = 0;

  display.clearDisplay();

  // 绘制背景网格
  for (int i = 0; i < 96; i += 8) {
    display.drawFastHLine(0, i, 96, 2);
    display.drawFastVLine(i, 0, 96, 2);
  }

  // 移动的方块
  display.fillRect(x, 20, 16, 16, 15);
  display.drawRect(x, 40, 16, 16, MONOOLED_WHITE);

  // 文本
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("Pos: %d", x);

  display.display();

  x = (x + 2) % 96;
  delay(32);
}
