#ifndef _SSD1327_GFX_H_
#define _SSD1327_GFX_H_

#include "Adafruit_GrayOLED.h"
#include <SPI.h>

#define SSD1327_WIDTH 96
#define SSD1327_HEIGHT 96
#define SSD1327_GRAY_LEVELS 16

#define SSD1327_SETCOLUMN 0x15
#define SSD1327_SETROW 0x75
#define SSD1327_SETCONTRAST 0x81
#define SSD1327_SETREMAP 0xA0
#define SSD1327_SETSTARTLINE 0xA1
#define SSD1327_SETOFFSET 0xA2
#define SSD1327_NORMALDISPLAY 0xA4
#define SSD1327_DISPLAYALLON 0xA5
#define SSD1327_DISPLAYALLOFF 0xA6
#define SSD1327_INVERTDISPLAY 0xA7
#define SSD1327_SETMULTIPLEX 0xA8
#define SSD1327_FUNCSEL 0xAB
#define SSD1327_DISPLAYOFF 0xAE
#define SSD1327_DISPLAYON 0xAF
#define SSD1327_SETPHASELEN 0xB1
#define SSD1327_SETCLOCK 0xB3
#define SSD1327_SETPRECHARGE 0xB6
#define SSD1327_SETGRAYTABLE 0xB8
#define SSD1327_SETVCOM 0xBE
#define SSD1327_SETPRECHARGE2 0xBC
#define SSD1327_FUNCSELB 0xD5

/*!
    @brief  Class for SSD1327 grayscale OLED displays
*/
class SSD1327_GFX : public Adafruit_GrayOLED {
public:
  SSD1327_GFX(uint8_t width, uint8_t height, SPIClass* spi,
    int8_t dc_pin, int8_t rst_pin, int8_t cs_pin,
    uint32_t bitrate = 8000000UL);

  ~SSD1327_GFX(void);

  bool begin(uint8_t i2caddr = 0x3C, bool reset = true);
  void display(void) override;

  // SSD1327特有命令
  void setContrast(uint8_t level);
  void setColumnAddress(uint8_t start = 8, uint8_t end = 55);
  void setRowAddress(uint8_t start = 0, uint8_t end = 95);
  void setGrayLevels(uint8_t levels);

private:
  SPIClass* _spi;
  uint8_t _width, _height;

  void writeCommand(uint8_t cmd);
  void writeData(uint8_t data);
  void writeDataBuffer(uint8_t* data, uint32_t len);
  void initDisplay(void);
};

#endif
