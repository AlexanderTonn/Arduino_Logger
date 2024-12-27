#include <Arduino.h>
#include <logger.hpp>

logger _loggerData;
SdFat _SdFat;

constexpr static uint8_t SPI_CS = 10;

uint16_t cycleNo = 0; 

void setup() {
  Serial.begin(115200);
  delay(2000);

 _loggerData.busInit(23, _SdFat);
 _loggerData.setupLogFile("LOG/DAT", logger::fileType::CSV, 10000, _SdFat);
 _loggerData.checkInit();

}

void loop() {
  _loggerData.logData("Test Data", _SdFat);

  delay(1000);
}