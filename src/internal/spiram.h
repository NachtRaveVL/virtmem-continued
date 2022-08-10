#ifndef VIRTMEM_SPIRAM_H
#define VIRTMEM_SPIRAM_H

/**
  * @file
  * @brief This file is a port of the serialram library to reduce dependency count.
  */

#include <Arduino.h>
#include <SPI.h>
#include "config/config.h"

#if defined(VIRTMEM_SPIRAM_USESPIFIFO) && (!defined(__arm__) || !defined(CORE_TEENSY))
#warning SPIFIFO only for teensy arm boards
#undef VIRTMEM_SPIRAM_USESPIFIFO
#endif
#ifdef VIRTMEM_SPIRAM_USESPIFIFO
#include <SPIFIFO.h>
#endif

namespace virtmem {

class SPISerialRam
{
    enum EInstruction
    {
        INSTR_READ = 0x03,
        INSTR_WRITE = 0x02,
        INSTR_RDMR = 0x05,
        INSTR_WRMR = 0x01
    };

    enum
    {
        SEQUENTIAL_MODE = 0x40,
        SPIFIFO_SIZE = 4
    };

    uint8_t _addrBytes;
    pintype_t _sramCSPin;
    SPISettings _spiSettings;
#ifdef VIRTMEM_SPIRAM_USESPIFIFO
    uint32_t _fifoSpeed;
#endif

    void initTransfer(EInstruction instruction);
    inline uint8_t sendByteMore(uint8_t byte) __attribute__((always_inline));
    inline uint8_t sendByteNoMore(uint8_t byte) __attribute__((always_inline));
    void sendAddress(uint32_t address);

public:
    SPISerialRam();
    SPISerialRam(uint32_t sramSize, pintype_t sramCSPin, uint32_t sramSpeed);

    void begin();
    void begin(uint32_t sramSize, pintype_t sramCSPin, uint32_t sramSpeed);

    void read(char *buffer, uint32_t address, uint32_t size);
    void write(const char *buffer, uint32_t address, uint32_t size);
};

}

#include "spiram.hpp"

#endif // VIRTMEM_SPIRAM_H
