#ifndef VIRTMEM_SERRAM_H
#define VIRTMEM_SERRAM_H

/**
  * @file
  * @brief This file is a copy of the serialram library to reduce dependency count.
  */

#include "config/config.h"

#if defined(VIRTMEM_SERRAM_USESPIFIFO) && (!defined(__arm__) || !defined(CORE_TEENSY))
#warning SPIFIFO only for teensy arm boards
#undef VIRTMEM_SERRAM_USESPIFIFO
#endif
#ifdef VIRTMEM_SERRAM_USESPIFIFO
#include <SPIFIFO.h>
#else
#include <SPI.h>
#endif

namespace virtmem {

class SerialRam
{
public:
    enum ESPISpeed { SPEED_FULL, SPEED_HALF, SPEED_QUARTER };

private:
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

    bool largeAddressing;
    uint8_t chipSelect;
    ESPISpeed SPISpeed;

    void initTransfer(EInstruction instruction);
    void endTransfer(void);
    inline uint8_t sendByteMore(uint8_t byte) __attribute__((always_inline));
    inline uint8_t sendByteNoMore(uint8_t byte) __attribute__((always_inline));
    void sendAddress(uint32_t address);

public:
    void begin(bool la, uint8_t pin, ESPISpeed speed);
    void end(void) { } // UNDONE(?)

    void read(char *buffer, uint32_t address, uint32_t size);
    void write(const char *buffer, uint32_t address, uint32_t size);
};

}

#endif // VIRTMEM_SERRAM_H
