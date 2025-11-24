#include <SPI.h>
#include "Common.h"
#include "DW1000Ranging.h"
#include "DW1000.h"

// ========== 配置选项 ==========
#define TAG_MODE
// #define ANCHOR_MODE

// ========== 设备参数 ==========
#ifdef TAG_MODE

// TAG antenna delay defaults to 16384
uint16_t shortAddress = 125; //7D:00
const char* macAddress = "22:EA:82:60:3B:9C";

// NOTE: Any change on this parameters will affect the transmission time of the packets
// So if you change this parameters you should also change the response times on the DW1000 library 
// (actual response times are based on experience made with these parameters)
static constexpr byte MY_MODE[] = { DW1000.TRX_RATE_110KBPS, DW1000.TX_PULSE_FREQ_16MHZ, DW1000.TX_PREAMBLE_LEN_1024 };

void newRange(DW1000Device* device) {
  Serial.print(device->getShortAddress(), HEX);
  Serial.print(",");
  Serial.println(device->getRange());
}

void newDevice(DW1000Device* device) {
  Serial.print("Device added: ");
  Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);
}


void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  delay(1000);

  //init the configuration
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  DW1000Ranging.init(BoardType::TAG, shortAddress, macAddress, false, MY_MODE, UWB_RST, UWB_CS, UWB_IRQ);

  DW1000.useSmartPower(false);
  DW1000.setReceiverAutoReenable(true);
  DW1000.suppressFrameCheck(false);  // 启用帧检查

  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
}

void loop() {
  DW1000Ranging.loop();
}


#elif defined(ANCHOR_MODE)

uint16_t shortAddress = 132; //84:00
const char* macAddress = "5B:D5:A9:9A:E2:9C";

//calibrated Antenna Delay setting for this anchor
uint16_t Adelay = 16630;

// previously determined calibration results for antenna delay
// #1 16630
// #2 16610
// #3 16607
// #4 16580

// calibration distance
float dist_m = (285 - 1.75) * 0.0254; //meters

// NOTE: Any change on this parameters will affect the transmission time of the packets
// So if you change this parameters you should also change the response times on the DW1000 library 
// (actual response times are based on experience made with these parameters)
static constexpr byte MY_MODE[] = { DW1000.TRX_RATE_110KBPS, DW1000.TX_PULSE_FREQ_16MHZ, DW1000.TX_PREAMBLE_LEN_1024 };

void newRange(DW1000Device* device) {
  //    Serial.print("from: ");
  Serial.print(device->getShortAddress(), HEX);
  Serial.print(", ");

#define NUMBER_OF_DISTANCES 1
  float dist = 0.0;
  for (int i = 0; i < NUMBER_OF_DISTANCES; i++) {
    dist += device->getRange();
  }
  dist = dist / NUMBER_OF_DISTANCES;
  Serial.println(dist);
}

void newDevice(DW1000Device* device) {
  Serial.print("Device added: ");
  Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("Delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);
}

void handleSerialCommands() {
  if (Serial.available()) {
    char command = Serial.read();

    switch (command) {
    case 'W':
      Adelay += 10;
      DW1000.setAntennaDelay(Adelay);
      Serial.print("已应用新延迟: ");
      Serial.println(Adelay);
      break;

    case 'S':
      Adelay -= 10;
      DW1000.setAntennaDelay(Adelay);
      Serial.print("已应用新延迟: ");
      Serial.println(Adelay);
      break;
    case 'w':
      Adelay += 1;
      DW1000.setAntennaDelay(Adelay);
      Serial.print("已应用新延迟: ");
      Serial.println(Adelay);
      break;

    case 's':
      Adelay -= 1;
      DW1000.setAntennaDelay(Adelay);
      Serial.print("已应用新延迟: ");
      Serial.println(Adelay);
      break;

    default:
      break;
    }
  }
}

void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  delay(1000); //wait for serial monitor to connect
  Serial.println("Anchor config and start");
  Serial.print("Antenna delay ");
  Serial.println(Adelay);
  Serial.print("Calibration distance ");
  Serial.println(dist_m);

  //init the configuration
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  DW1000Ranging.init(BoardType::ANCHOR, shortAddress, macAddress, false, MY_MODE, UWB_RST, UWB_CS, UWB_IRQ);

  // set antenna delay for anchors only. Tag is default (16384)
  DW1000.setAntennaDelay(Adelay);

  DW1000.useSmartPower(false);
  DW1000.setReceiverAutoReenable(true);
  DW1000.suppressFrameCheck(false);  // 启用帧检查

  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
}

void loop() {
  DW1000Ranging.loop();
  handleSerialCommands();
}
#else

#endif
