#include <Arduino.h>
#include <logger.hpp>

logger _loggerError;
SdFat _SdFat;

constexpr static uint8_t SPI_CS = 10;

uint16_t cycleNo = 0; 

void setup() {
  Serial.begin(115200);
  delay(2000);

 _loggerError.busInit(23, _SdFat);
 _loggerError.setupLogFile("LOG/ERR", logger::fileType::TXT, 10000, _SdFat);
 _loggerError.checkInit();

}

void loop() {
  _loggerError.logData("Test String", _SdFat);
  
  delay(1000);
}