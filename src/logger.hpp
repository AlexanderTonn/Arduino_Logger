/**
 * @file msc.hpp
 * @author Alexander Tonn
 * @brief mass storage class
 * @version 1.0
 * @date 2024-06-25
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <Arduino.h>
#include "SdFat.h"
class logger
{
private:
    constexpr static uint32_t SPI_SPEED = SD_SCK_MHZ(4);
    bool mOperational = false; // Flag for the operational state of the SD Card
    bool mSetupLogFileDone = false; // Flag for the log file setup

    String mFilePath = "";
    String mFileExtension = "";
    int32_t mFileIndex = 0; // Index for the error log file
    uint32_t mMaxFileSize = 0; // Max file size for the log file
    File mFile ;

public:
    auto busInit(uint8_t, SdFat&) -> void;
    auto checkInit() -> void;
    auto close(String*, SdFat&) -> void;

    enum class fileType
    {
        CSV,
        TXT
    };

    auto logData(const char*, SdFat&) -> void;
    auto setupLogFile(String, fileType, const uint32_t, SdFat&) -> void;

private: 
    constexpr static uint8_t ARRAY_SIZE = 10; 
    constexpr static uint8_t STRING_LENGTH = 50;
    char mBuffer[ARRAY_SIZE][STRING_LENGTH]; // Buffer for the log entries
    
    uint8_t mArrayMaxPosition = ARRAY_SIZE - 1;
    uint8_t mArrayPosition = 0;

    auto writeToBuffer(const char*, SdFat&) -> void;
    auto getNextFileIndex(File,int32_t &) -> uint16_t;
    auto clearBuffer() -> void; 
    auto push_back(const char*) -> void;

    enum class writeStep
    {
        CHECK_INDEX,
        WRITE_FILE,
        CHECK_SIZE
    };
    writeStep mWriteStep = writeStep::CHECK_SIZE;

    auto writeBufferToFile(SdFat&) -> bool;


    public:
    auto block(bool) -> void;


};

#endif // MSC_HPP