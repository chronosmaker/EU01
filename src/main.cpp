#include "Common.h"
#include "SSD1327_GFX.h"
#include <math.h>  // 用于三角函数计算角度

// 创建SSD1327显示对象
SSD1327_GFX display(96, 96, &SPI, OLED_DC, OLED_RESET, OLED_CS, 8000000);

// 方块参数配置
const int SQUARE_SIZE = 8;     // 方块大小（像素）
const float SPEED = 3;       // 移动速度（像素/帧）
float x, y;                    // 方块左上角坐标
float vx, vy;                  // 速度向量

void setup() {
  Serial.begin(9600);
  Serial0.begin(9600);

  if (!display.begin()) {
    Serial.println("SSD1327初始化失败!");
    while (1);
  }
  Serial.println("动态方块示例启动");

  // 初始化方块位置（屏幕中央）
  x = (96 - SQUARE_SIZE) / 2.0;
  y = (96 - SQUARE_SIZE) / 2.0;

  // 生成随机斜向初始方向（角度范围30°-60°，避免过于接近轴）
  float angle = random(30, 61) * PI / 180.0;  // 转换为弧度
  if (random(2) == 0) angle += PI / 2;        // 增加90°变体以覆盖更多方向
  vx = SPEED * cos(angle);
  vy = SPEED * sin(angle);
  // 随机选择象限
  if (random(2) == 0) vx = -vx;
  if (random(2) == 0) vy = -vy;

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
}

void loop() {
  if (Serial0.available()) {
    Serial.write(Serial0.read());
  }

  display.clearDisplay();

  // 可选：绘制背景网格（低灰度）
  for (int i = 0; i < 96; i += 16) {
    display.drawFastHLine(0, i, 96, 1);
    display.drawFastVLine(i, 0, 96, 1);
  }

  // 更新方块位置
  x += vx;
  y += vy;

  // 碰撞检测与镜面反射
  if (x <= 0) {           // 左边缘
    x = 0;
    vx = -vx;
  } else if (x + SQUARE_SIZE >= 96) {  // 右边缘
    x = 96 - SQUARE_SIZE;
    vx = -vx;
  }
  if (y <= 0) {           // 上边缘
    y = 0;
    vy = -vy;
  } else if (y + SQUARE_SIZE >= 96) {  // 下边缘
    y = 96 - SQUARE_SIZE;
    vy = -vy;
  }

  // 绘制方块（高灰度）和轨迹点（中等灰度）
  display.fillRect((int)x, (int)y, SQUARE_SIZE, SQUARE_SIZE, 15);
  display.drawPixel((int)(x + SQUARE_SIZE / 2), (int)(y + SQUARE_SIZE / 2), 8);

  // 显示状态信息
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("x:%.2f y:%.2f", x, y);

  display.display();
  delay(20);  // 控制帧率（约50FPS）
}
