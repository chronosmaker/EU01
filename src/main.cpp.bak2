
#include <Arduino.h>
#include <SPI.h>

// ESP32-S3 SPI引脚定义
#define OLED_CLK    7  // SCK
#define OLED_MOSI   9  // MOSI  
#define OLED_CS     6   // 片选
#define OLED_DC     3   // 数据/命令
#define OLED_RESET  4   // 复位

// SSD1327配置
#define OLED_WIDTH   96
#define OLED_HEIGHT  96
#define GRAY_LEVELS  16

class SSD1327 {
private:
  SPIClass* spi;

  void init() {
    // 硬件复位
    digitalWrite(OLED_RESET, LOW);
    delay(10);
    digitalWrite(OLED_RESET, HIGH);
    delay(100);

    // 初始化命令序列
    command(0xFD);  // 命令锁定解锁
    command(0x12);

    command(0xAE);  // 显示关闭

    // 设置96x96显示区域
    command(0x15);  // 设置列地址
    command(8);    // 起始列 = 16 (128-96)/2 = 16
    command(55);   // 结束列 = 16+95 = 111

    command(0x75);  // 设置行地址  
    command(0);    // 起始行 = 16 (128-96)/2 = 16
    command(95);   // 结束行 = 16+95 = 111

    command(0xA0);  // 重映射设置
    command(0x56);  // 水平地址增量, COM重映射, 分割奇偶COM

    command(0xA1);  // 显示起始行
    command(0x00);

    command(0xA2);  // 显示偏移
    command(0x00);

    command(0xA8);  // 复用比率
    command(0x7f);    // 96 MUX (95+1)

    command(0xB1);  // 相位长度
    command(0x51);  // 阶段1=5, 阶段2=1

    command(0xB3);  // 显示时钟分频/振荡器频率
    command(0x91);  // 分频=1, 频率=9

    command(0xAB);  // 功能选择A
    command(0x01);  // 使能内部VDD调节器

    command(0xB6);  // 第二预充电周期
    command(0x0F);  // 15个DCLK

    command(0xBE);  // VCOMH电压
    command(0x0F);  // 0.86 x VCC

    command(0xBC);  // 预充电电压
    command(0x08);  // VCOMH水平

    command(0xD5);  // 功能选择B
    command(0x62);  // 使能第二预充电, 内部VSL

    command(0x81);
    command(0x80);
    // 设置16级灰度表（线性）
    command(0xB8);
    command(0x00); // GS1
    command(0x02); // GS2  
    command(0x04); // GS3
    command(0x06); // GS4
    command(0x08); // GS5
    command(0x0A); // GS6
    command(0x0C); // GS7
    command(0x0E); // GS8
    command(0x10); // GS9
    command(0x12); // GS10
    command(0x14); // GS11
    command(0x16); // GS12
    command(0x18); // GS13
    command(0x1A); // GS14
    command(0x1C); // GS15

    command(0xA4); // 正常显示模式
    command(0xAF); // 显示开启
  }

public:
  void begin() {
    // 初始化SPI
    spi = new SPIClass(HSPI);
    spi->begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);

    // 设置引脚模式
    pinMode(OLED_CS, OUTPUT);
    pinMode(OLED_DC, OUTPUT);
    pinMode(OLED_RESET, OUTPUT);

    // 初始化显示
    init();
  }

  void command(uint8_t cmd) {
    digitalWrite(OLED_DC, LOW);
    digitalWrite(OLED_CS, LOW);
    spi->transfer(cmd);
    digitalWrite(OLED_CS, HIGH);
  }

  void data(uint8_t data) {
    digitalWrite(OLED_DC, HIGH);
    digitalWrite(OLED_CS, LOW);
    spi->transfer(data);
    digitalWrite(OLED_DC, HIGH);
  }

  // 设置显示区域（96x96）
  void setDisplayArea(uint8_t startCol, uint8_t endCol, uint8_t startRow, uint8_t endRow) {
    command(0x15); // 列地址
    command(startCol);
    command(endCol);

    command(0x75); // 行地址
    command(startRow);
    command(endRow);
  }

  // 清除屏幕
  void clear() {
    // setDisplayArea(8, 55, 0, 95); // 96x96区域

    digitalWrite(OLED_DC, HIGH);
    digitalWrite(OLED_CS, LOW);

    for (uint32_t i = 0; i < (96 * 96 / 2); i++) {
      spi->transfer(0x00); // 每个字节包含2个像素，都设为0（黑色）
    }

    digitalWrite(OLED_CS, HIGH);
  }

  // 批量传输显示数据（推荐使用）
  void display(const uint8_t* buffer) {
    // setDisplayArea(8, 55, 0, 95);

    digitalWrite(OLED_DC, HIGH);
    digitalWrite(OLED_CS, LOW);

    // 传输96x96/2 = 4608字节
    for (uint32_t i = 0; i < (96 * 96 / 2); i++) {
      spi->transfer(buffer[i]);
    }

    digitalWrite(OLED_CS, HIGH);
  }
};

SSD1327 oled;

void setup() {
  Serial.begin(115200);
  oled.begin();
  oled.clear();

  // 创建测试图案
  uint8_t buffer[96 * 96 / 2]; // 96x96像素，每像素4位，每字节2像素

  // 填充渐变图案
  for (int y = 0; y < 96; y++) {
    for (int x = 0; x < 96; x += 2) {
      uint8_t gray1 = (x + y) % 8;
      uint8_t gray2 = (x + 1 + y) % 8;
      buffer[(y * 48) + (x / 2)] = (gray1 << 4) | gray2;
    }
  }

  oled.display(buffer);
  delay(1000);
}

void loop() {
  // 动态显示内容
}