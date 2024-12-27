#include "logger.hpp"

/**
 * @brief Init the SD Card bus
 * @note call this once if you only have one sd card module
 * @param CS_PIN
 * @param _SdFat
 */
auto logger::busInit(uint8_t CS_PIN, SdFat &_SdFat) -> void
{
    // Exit if no serial connection
    if (!Serial)
        return;

    // Exit if sd module have troubles
    if (!_SdFat.begin(CS_PIN, SPI_SPEED))
    {
        if (_SdFat.card()->errorCode())
        {
            Serial.print("SD Card Error: ");
            Serial.println(_SdFat.card()->errorCode());
            return;
        }
    }

    if (_SdFat.vol()->fatType() == 0)
    {
        Serial.println("Invalid Filesystem");
    }
}

/**
 * @brief check function to see if everything is set up correctly
 *
 */
auto logger::checkInit() -> void
{
    // Exit if no log file path is set
    if (!mSetupLogFileDone)
    {
        Serial.println("Run setupLogFile() first");
        return;
    }
    mOperational = true;
}
/**
 * @brief close the SD Card bus
 * @note destroy the buffer and close the SD Card bus
 * @param *buffer pointer to buffer
 * @param &_SdFat Reference to the SdFat object
 */
auto logger::close(String *buffer, SdFat &_SdFat) -> void
{
    if (!_SdFat.isBusy() && mOperational)
    {
        _SdFat.end();
        mOperational = false;
        mArrayPosition = 0;
        mSetupLogFileDone = false;
    }
}
/**
 * @brief main function for saving logging data
 *
 * @param data Your logging message
 * @param &_sdFat Reference to the SdFat object
 */
auto logger::logData(const char *data, SdFat &_sdFat) -> void
{

    if (sizeof(data) > (STRING_LENGTH - 20) * sizeof(char))
    {
        Serial.println("Data too long");
        return;
    }
    char logString[STRING_LENGTH] = {0};
    snprintf(logString, STRING_LENGTH, "%lu ms ; %s \n", millis(), data);
    writeToBuffer(logString, _sdFat);
}

/**
 * @brief setup the log file
 *
 * @param filePath String with the path to target DIRECTORY
 * @param _fileType enum class of logger class
 * @param maxFileSize max size in bytes for each file, if it's greater than this size, a new file will be created
 * @param &_SdFat Reference to the SdFat object
 */
auto logger::setupLogFile(String filePath, fileType _fileType, const uint32_t maxFileSize, SdFat &_SdFat) -> void
{
    mFilePath = filePath;
    mMaxFileSize = maxFileSize;

    // Check whether dir exists and abort if dir could not be created
    if (!_SdFat.exists(mFilePath.c_str()))
    {
        if (!_SdFat.mkdir(mFilePath.c_str()))
        {
            mSetupLogFileDone = false;
            return;
        }
    }
    mSetupLogFileDone = true;

    switch (_fileType)
    {
    case fileType::CSV:
        mFileExtension = ".csv";
        break;
    case fileType::TXT:
        mFileExtension = ".txt";
        break;
    }

    Serial.println("SD Init Done");
    Serial.println(_SdFat.ls(LS_R | LS_DATE | LS_SIZE));
}
/**
 * @brief Clean up the buffer
 *
 */
auto logger::clearBuffer() -> void
{
    for (auto i = 0; i < mArrayPosition; i++)
    {
        memset(mBuffer[i], 0, sizeof(mBuffer[i]));
    }
    mArrayPosition = 0;
}
/**
 * @brief Write the data to the buffer
 *
 * @param data String with the data
 * @param &_sdFat Reference to the SdFat object
 */
auto logger::writeToBuffer(const char *data, SdFat &_sdFat) -> void
{
    if (mArrayPosition < mArrayMaxPosition)
        push_back(data);

    if (mArrayPosition >= mArrayMaxPosition)
    {
        block(true);
        writeBufferToFile(_sdFat);
    }
}
/**
 * @brief push back the data to the internal buffer
 *
 * @param data String with the data
 */
auto logger::push_back(const char *data) -> void
{
    if (!mOperational)
        return;

    strncpy(mBuffer[mArrayPosition], data, sizeof(mBuffer[mArrayPosition]));
    mArrayPosition++;
}
/**
 * @brief Write the buffer to the file
 *
 * @param &_SdFat Reference to the SdFat object
 * @return bool true if file was written
 */
auto logger::writeBufferToFile(SdFat &_SdFat) -> bool
{
    bool ret = false;

    String fullPath = mFilePath + "/" + String(mFileIndex) + mFileExtension;
    File dir; 

    if (!mFile.isOpen())
        mFile = _SdFat.open(fullPath.c_str(), FILE_WRITE | O_CREAT | O_APPEND);

    switch (mWriteStep)
    {
    case writeStep::CHECK_SIZE:
        mWriteStep = mFile.fileSize() > mMaxFileSize ? writeStep::CHECK_INDEX : writeStep::WRITE_FILE;

        ret = false;
        break;

    case writeStep::CHECK_INDEX:
        if (mFile.isOpen())
            mFile.close();

        dir = _SdFat.open(mFilePath.c_str());
        getNextFileIndex(dir, mFileIndex);
        dir.close();

        mWriteStep = writeStep::CHECK_SIZE;

        ret = false;
        break;
    case writeStep::WRITE_FILE:
        // SD interface busy currently
        if (!_SdFat.isBusy())
        {
            for (auto i = 0; i < mArrayPosition; i++)
            {
                mFile.print(mBuffer[i]);
            }

            if (mFile.isOpen())
            {
                mFile.close();
                mArrayPosition = 0; // Reset the buffer position
                ret = true;         // Success
                block(false);
                clearBuffer();
            }

            mWriteStep = writeStep::CHECK_SIZE;
        }

        break;

    default:
        break;
    }

    return ret;
}
/**
 * @brief get the next file index
 *
 * @param dir File object of the directory
 * @param &currentIndex current index of the file
 * @return uint16_t
 */
auto logger::getNextFileIndex(File dir, int32_t &currentIndex) -> uint16_t
{
    char fileName[20] = {0};

    while (true)
    {
        File entry = dir.openNextFile();

        // No furhtermore entries in the dir
        if (!entry)
        {
            entry.close();
            break; // Exit loop
        }

        // File is a directory, i only want to know about files
        if (entry.isDirectory())
        {
            entry.close();
            return -2;
        }

        entry.getName(fileName, sizeof(fileName));
        entry.close();
    }

    // Extract the index from the filename
    String charNumbers;
    for (auto c : fileName)
    {
        if (isDigit(c))
            charNumbers += c;
    }

    currentIndex = charNumbers.toInt() + 1;

    return 1; // Success
}
/**
 * @brief block the SD Interface for write and read actions while the bool is true
 *
 */
auto logger::block(bool block) -> void
{
    mOperational = block ? false : true;
}