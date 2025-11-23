#include "SSD1327_GFX.h"
#include <Arduino.h>

// 构造函数
SSD1327_GFX::SSD1327_GFX(uint8_t width, uint8_t height, SPIClass* spi,
  int8_t dc_pin, int8_t rst_pin, int8_t cs_pin,
  uint32_t bitrate)
  : Adafruit_GrayOLED(4, width, height, spi, dc_pin, rst_pin, cs_pin, bitrate),
  _spi(spi), _width(width), _height(height) {
}

// 析构函数
SSD1327_GFX::~SSD1327_GFX(void) {
}

// 初始化显示
bool SSD1327_GFX::begin(uint8_t i2caddr, bool reset) {
  // 调用父类初始化（分配缓冲区等）
  if (!_init(i2caddr, reset)) {
    return false;
  }

  // 初始化SPI引脚
  pinMode(csPin, OUTPUT);
  pinMode(dcPin, OUTPUT);
  if (rstPin >= 0) {
    pinMode(rstPin, OUTPUT);
  }

  // 初始化显示
  initDisplay();

  // 清除显示并设置初始状态
  clearDisplay();
  display();

  return true;
}

// 初始化显示命令序列
void SSD1327_GFX::initDisplay(void) {
  // 硬件复位
  if (rstPin >= 0) {
    digitalWrite(rstPin, LOW);
    delay(10);
    digitalWrite(rstPin, HIGH);
    delay(100);
  }

  // 初始化命令序列
  writeCommand(0xFD);  // 命令锁定解锁
  writeCommand(0x12);

  writeCommand(SSD1327_DISPLAYOFF);  // 显示关闭

  // 设置显示区域 (96x96，居中于128x128)
  setColumnAddress(8, 55);  // 列地址 8-55 (共48字节，96像素)
  setRowAddress(0, 95);     // 行地址 0-95

  writeCommand(SSD1327_SETREMAP);    // 重映射设置
  writeCommand(0x51);                // 水平地址增量, COM重映射

  writeCommand(SSD1327_SETSTARTLINE); // 显示起始行
  writeCommand(0x00);

  writeCommand(SSD1327_SETOFFSET);   // 显示偏移
  writeCommand(0x00);

  writeCommand(SSD1327_SETMULTIPLEX); // 复用比率
  writeCommand(0x7F);                 // 96 MUX (95+1)

  writeCommand(SSD1327_SETPHASELEN);  // 相位长度
  writeCommand(0x51);                 // 阶段1=5, 阶段2=1

  writeCommand(SSD1327_SETCLOCK);     // 显示时钟分频
  writeCommand(0x91);                 // 分频=1, 频率=9

  writeCommand(SSD1327_FUNCSEL);      // 功能选择A
  writeCommand(0x01);                 // 使能内部VDD调节器

  writeCommand(SSD1327_SETPRECHARGE); // 第二预充电周期
  writeCommand(0x0F);                 // 15个DCLK

  writeCommand(SSD1327_SETVCOM);      // VCOMH电压
  writeCommand(0x0F);                 // 0.86 x VCC

  writeCommand(0xBC);                 // 预充电电压
  writeCommand(0x08);                 // VCOMH水平

  writeCommand(0xD5);                 // 功能选择B
  writeCommand(0x62);                 // 使能第二预充电, 内部VSL

  // 设置对比度
  setContrast(0x80);

  // 设置16级灰度表
  setGrayLevels(SSD1327_GRAY_LEVELS);

  writeCommand(SSD1327_NORMALDISPLAY); // 正常显示模式
  writeCommand(SSD1327_DISPLAYON);     // 显示开启
}

// 显示函数 - 将缓冲区内容发送到显示屏
void SSD1327_GFX::display(void) {
  // 发送数据到显示RAM
  writeCommand(0x5C); // 写入RAM命令

  // 由于SSD1327是4位灰度，我们的缓冲区已经是4位格式
  // 直接发送整个缓冲区
  digitalWrite(dcPin, HIGH);
  digitalWrite(csPin, LOW);

  // 传输数据：96x96像素，每像素4位，每字节2个像素
  uint32_t bufferSize = (_width * _height) / 2;
  _spi->transferBytes(buffer, NULL, bufferSize);

  digitalWrite(csPin, HIGH);
}

// 设置对比度
void SSD1327_GFX::setContrast(uint8_t level) {
  writeCommand(SSD1327_SETCONTRAST);
  writeCommand(level);
}

// 设置列地址
void SSD1327_GFX::setColumnAddress(uint8_t start, uint8_t end) {
  writeCommand(SSD1327_SETCOLUMN);
  writeCommand(start);
  writeCommand(end);
}

// 设置行地址  
void SSD1327_GFX::setRowAddress(uint8_t start, uint8_t end) {
  writeCommand(SSD1327_SETROW);
  writeCommand(start);
  writeCommand(end);
}

// 设置16级灰度表（线性）
void SSD1327_GFX::setGrayLevels(uint8_t levels) {
  writeCommand(SSD1327_SETGRAYTABLE);
  // 线性灰度表
  for (uint8_t i = 0; i < levels; i++) {
    writeCommand(i * 2);
  }
}

// 底层命令写入函数
void SSD1327_GFX::writeCommand(uint8_t cmd) {
  digitalWrite(dcPin, LOW);
  digitalWrite(csPin, LOW);
  _spi->transfer(cmd);
  digitalWrite(csPin, HIGH);
}

void SSD1327_GFX::writeData(uint8_t data) {
  digitalWrite(dcPin, HIGH);
  digitalWrite(csPin, LOW);
  _spi->transfer(data);
  digitalWrite(csPin, HIGH);
}
