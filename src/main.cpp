#include <Arduino.h>
#include <U8g2lib.h> 

// 配置U8g2库用于SSD1327控制器，96x96分辨率，硬件SPI
// 参数依次为：旋转方向、片选引脚、数据命令引脚、复位引脚
U8G2_SSD1327_SEEED_96X96_1_4W_HW_SPI u8g2(U8G2_R0, 4, 3, 2);


void setup() {

  Serial.begin(9600);
  Serial1.begin(9600);
  u8g2.setBusClock(8000000); // 设置SPI时钟为8MHz（需屏幕支持）
  u8g2.setContrast(128); // 设置对比度（0-255，灰度屏需调整）
  u8g2.setFont(u8g2_font_helvB08_tr); // 设置字体
  u8g2.begin(); // 初始化显示屏
  u8g2.clearBuffer();   // 清空缓冲区
}

void drawGrayBox(int x, int y, int w, int h, int level) {
  for (int i = 0; i < level; i++) {
    u8g2.drawFrame(x + i, y + i, w - 2 * i, h - 2 * i); // 层级越多灰度越深
  }
}

void loop() {
  if (Serial.available()) {        // If anything comes in Serial (USB),
    Serial.write(Serial.read());  // read it and send it out Serial1 (pins 0 & 1)
  }
  if (Serial1.available()) {       // If anything comes in Serial1 (pins 0 & 1)
    Serial.write(Serial1.read());  // read it and send it out Serial (USB)
  }

  // 启动画面绘制循环
  u8g2.firstPage();
  // 在循环内使用绘制函数描述要显示的内容
  do {
    // 示例1：显示文本（16级灰度需使用支持灰度的字体）
    u8g2.drawStr(0, 20, "Hello SSD1327"); // 在坐标(0,20)绘制文本
    // 示例2：绘制灰度图形（矩形框和填充框）
    u8g2.drawFrame(10, 30, 40, 20);      // 空心矩形（x,y,宽,高）
    u8g2.drawBox(60, 30, 40, 20);        // 实心矩形
    // 示例3：利用灰度特性绘制渐变图形（通过重复绘制实现）
    for (int i = 0; i < 4; i++) {
      u8g2.drawCircle(48 + i, 70 + i, 10); // 绘制重叠圆形模拟灰度
    }
    // 绘制中灰色方块
    drawGrayBox(10, 50, 30, 30, 4);
  } while (u8g2.nextPage()); // 循环结束，将内容发送到显示屏

  delay(1000);
}

