#ifndef VIRTMEM_SD_ALLOC_H
#define VIRTMEM_SD_ALLOC_H

/**
  * @file
  * @brief This file contains the SD virtual memory allocator
  */

#include <SD.h>
#include "internal/alloc.h"

namespace virtmem {

#ifdef ESP32
typedef SDFileSystemClass SDClass;
#endif

#ifndef SD_CHIP_SELECT_PIN
#ifndef SDCARD_SS_PIN
#define SD_CHIP_SELECT_PIN SS
#else
#define SD_CHIP_SELECT_PIN SDCARD_SS_PIN
#endif
#endif

#ifndef SD_SPI_SPEED
#ifndef SPI_FULL_SPEED
#define SD_SPI_SPEED 50000000
#else
#define SD_SPI_SPEED SPI_FULL_SPEED
#endif
#endif

/**
 * @brief Virtual memory allocator class that uses SD card as virtual pool
 *
 * This class uses a file on a FAT32 formatted SD card as virtual memory pool. The
 * platform SD library is used to interface with the SD card.
 *
 * When the allocator is initialized (i.e. by calling start()) it will create a file called
 * 'ramfile.vm' in the root directory. Existing files will be reused and resized if necessary.
 *
 * @tparam Properties Allocator properties, see DefaultAllocProperties
 *
 * @sa @ref bUsing, SDVAlloc
 */
template <typename Properties=DefaultAllocProperties>
class SDVAllocP : public VAlloc<Properties, SDVAllocP<Properties> >
{
    pintype_t _sdCSPin;
#ifndef CORE_TEENSY
    uint32_t _sdSpeed;
#endif
    SDClass *_sdClass;
#ifdef ESP32
    SPIClass *_spiClass;
#endif
    File _ramFile;
    String _ramFilename;
    bool _sdBegan;

    inline void beginSD()
    {
#if defined(ESP32)
        _sdBegan = _sdClass->begin(_sdCSPin, *_spiClass, _sdSpeed);
#elif defined(CORE_TEENSY)
        _sdBegan = _sdClass->begin(_sdCSPin); // card speed not possible to set on teensy
#else
        _sdBegan = _sdClass->begin(_sdSpeed, _sdCSPin);
#endif
        ASSERT(_sdBegan);
    }

    void doStart(void)
    {
        if (!_sdBegan) { beginSD(); }
        if (_sdBegan) {
            _ramFile = _sdClass->open(_ramFilename.c_str(), FILE_WRITE);

            // make file the right size if needed
            const uint32_t size = _ramFile.size();
            if (size < this->getPoolSize()) {
                this->writeZeros(size, this->getPoolSize() - size);
            }
        }
    }

    void doStop(void)
    {
        if (_sdBegan) {
            if (_ramFile) { _ramFile.close(); }
            #if !defined(CORE_TEENSY) // no delayed write on teensy's SD impl
                _sdClass->end();
            #endif
            _sdBegan = false;
        }
    }

    void doRead(void *data, VPtrSize offset, VPtrSize size)
    {
//        const uint32_t t = micros();
        _ramFile.seek(offset);
        _ramFile.read((uint8_t *)data, size);
//        Serial.print("read: "); Serial.print(size); Serial.print("/"); Serial.println(micros() - t);
    }

    void doWrite(const void *data, VPtrSize offset, VPtrSize size)
    {
//        const uint32_t t = micros();
        _ramFile.seek(offset);
        _ramFile.write((const uint8_t *)data, size);
//        Serial.print("write: "); Serial.print(size); Serial.print("/"); Serial.println(micros() - t);
    }

public:
    /** Constructs (but not initializes) the SD FAT32 allocator.
     * @param ps The size of the virtual memory pool
     * @param sdCSPin The CS pin of the SD card
     * @param sdSpeed The speed of the SPI connection (except Teensy)
     * @param sdClass Class instance for SD interface
     * @param spiClass Class instance for SPI interface (ESP32 only)
     * @sa setPoolSize
     */
    SDVAllocP(VPtrSize ps = VIRTMEM_DEFAULT_POOLSIZE,
              pintype_t sdCSPin = SD_CHIP_SELECT_PIN,
              uint32_t sdSpeed = SD_SPI_SPEED,
#if (!defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SD)) || defined(SD)
              SDClass *sdClass = &SD
#else
              SDClass *sdClass = new SDClass()
#endif
#ifdef ESP32
              , SPIClass *spiClass = &SPI
#endif
    )   : _sdCSPin(sdCSPin),
#ifndef CORE_TEENSY
          _sdSpeed(sdSpeed),
#endif
#ifdef ESP32
          _spiClass(spiClass),
#endif
          _sdClass(sdClass), _ramFilename(F("ramfile.vm")), _sdBegan(false)
    {
        this->setPoolSize(ps);
    }
    ~SDVAllocP(void) { doStop(); }

    /**
     * Removes the RAM file used as virtual memory pool.
     * @note Only call this when the allocator is not initialized!
     */
    void removeRAMFile(void)
    {
        ASSERT(!_ramFile);
        if (!_sdBegan) { beginSD(); }
        if (_sdBegan) { _sdClass->remove(_ramFilename.c_str()); }
    }
};

typedef SDVAllocP<> SDVAlloc; //!< Shortcut to SDVAllocP with default template arguments

/**
 * @example sd_simple.ino
 * This is a simple example sketch showing how to use the SD allocator.
 */

}

#endif // VIRTMEM_SD_ALLOC_H
