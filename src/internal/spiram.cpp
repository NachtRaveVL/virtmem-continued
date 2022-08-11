#include "spiram.h"

#ifdef VIRTMEM_SPIRAM_USENODATASWAP
namespace {

inline uint16_t getWord(uint8_t h, uint8_t l) { return h << 8 | l; }

// recent teensyduino version supports toolchain with __builtin_bswap16
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION >= 40800
inline uint16_t swapInt16(uint16_t x) { return __builtin_bswap16(x); }
#else
inline uint16_t swapInt16(uint16_t x) { return (x >> 8) | ((x & 0xff) << 8); }
#endif
#undef GCC_VERSION

}
#endif

namespace virtmem {

void SPISerialRam::initTransfer(EInstruction instruction)
{
    SPI.beginTransaction(_spiSettings);

#ifdef VIRTMEM_SPIRAM_USESPIFIFO
    SPIFIFO.begin(_sramCSPin, _fifoSpeed);
#else
    digitalWrite(_sramCSPin, LOW);
#endif

    sendByteMore(instruction);
}

void SPISerialRam::sendAddress(uint32_t address)
{
    switch(_addrBytes) {
        case 4:
            sendByteMore((uint8_t)(address >> 24));
        case 3:
            sendByteMore((uint8_t)(address >> 16));
        case 2:
            sendByteMore((uint8_t)(address >> 8));
        default:
            sendByteMore((uint8_t)address);
    }
}

SPISerialRam::SPISerialRam()
{ }

SPISerialRam::SPISerialRam(uint32_t sramSize, pintype_t sramCSPin, uint32_t sramSpeed)
    : _addrBytes((sramSize >= 0x1U       ? 1 : 0) +
                 (sramSize >= 0x100U     ? 1 : 0) +
                 (sramSize >= 0x10000U   ? 1 : 0) +
                 (sramSize >= 0x1000000U ? 1 : 0)),
      _sramCSPin(sramCSPin),
      _spiSettings(sramSpeed, MSBFIRST, SPI_MODE0)
#ifdef VIRTMEM_SPIRAM_USESPIFIFO
      , _fifoSpeed( sramSpeed >= 24000000U ? SPI_CLOCK_24MHz :
                   (sramSpeed >= 16000000U ? SPI_CLOCK_16MHz :
                   (sramSpeed >= 12000000U ? SPI_CLOCK_12MHz :
                   (sramSpeed >= 8000000U  ? SPI_CLOCK_8MHz :
                   (sramSpeed >= 6000000U  ? SPI_CLOCK_6MHz :
                                             SPI_CLOCK_4MHz)))))
#endif
#ifdef VIRTMEM_SPIRAM_CAPTURESPEED
      , _sramSpeed(sramSpeed)
#endif
{ }

void SPISerialRam::begin(uint32_t sramSize, pintype_t sramCSPin, uint32_t sramSpeed)
{
    _addrBytes = (sramSize >= 0x1U       ? 1 : 0) +
                 (sramSize >= 0x100U     ? 1 : 0) +
                 (sramSize >= 0x10000U   ? 1 : 0) +
                 (sramSize >= 0x1000000U ? 1 : 0);
    _sramCSPin = sramCSPin;
    _spiSettings = SPISettings(sramSpeed, MSBFIRST, SPI_MODE0);
#ifdef VIRTMEM_SPIRAM_USESPIFIFO
    _fifoSpeed =  sramSpeed >= 24000000U ? SPI_CLOCK_24MHz :
                 (sramSpeed >= 16000000U ? SPI_CLOCK_16MHz :
                 (sramSpeed >= 12000000U ? SPI_CLOCK_12MHz :
                 (sramSpeed >= 8000000U  ? SPI_CLOCK_8MHz :
                 (sramSpeed >= 6000000U  ? SPI_CLOCK_6MHz :
                                           SPI_CLOCK_4MHz))));
#endif

    begin();
}

void SPISerialRam::begin()
{
    SPI.begin();

#ifndef VIRTMEM_SPIRAM_USESPIFIFO
    pinMode(_sramCSPin, OUTPUT);
    digitalWrite(_sramCSPin, HIGH);
#endif

    // default to sequential mode
    initTransfer(INSTR_WRMR);
    sendByteNoMore(SEQUENTIAL_MODE);
}

void SPISerialRam::read(char *buffer, uint32_t address, uint32_t size)
{
    initTransfer(INSTR_READ);
    sendAddress(address);

#ifdef VIRTMEM_SPIRAM_USESPIFIFO
    if (size & 1)
    {
        if (size == 1)
        {
            *buffer = sendByteNoMore(0xFF);
            return;
        }
        else
            *buffer++ = sendByteMore(0xFF);
        --size;
    }

    const bool usefifo = size > (SPIFIFO_SIZE*2);
    if (usefifo)
    {
        // fill FIFO
        for (uint8_t i=0; i<(SPIFIFO_SIZE-1); ++i)
            SPIFIFO.write16(0xFFFF, SPI_CONTINUE);
        size -= ((SPIFIFO_SIZE-1)*2);
    }

    for (; size != 2; size-=2, buffer+=2)
    {
        SPIFIFO.write16(0xFFFF, SPI_CONTINUE);
#ifdef VIRTMEM_SPIRAM_USENODATASWAP
        *(uint16_t *)buffer = swapInt16(SPIFIFO.read());
#else
        *(uint16_t *)buffer = SPIFIFO.read();
#endif
    }

    SPIFIFO.write16(0xFFFF, 0);
#ifdef VIRTMEM_SPIRAM_USENODATASWAP
    *(uint16_t *)buffer = swapInt16(SPIFIFO.read());
#else
    *(uint16_t *)buffer = SPIFIFO.read();
#endif
    buffer += 2;

    if (usefifo)
    {
        // purge FIFO
#ifdef VIRTMEM_SPIRAM_USENODATASWAP
        for (uint8_t i=0; i<(SPIFIFO_SIZE-1); ++i, buffer+=2)
            *(uint16_t *)buffer = swapInt16(SPIFIFO.read());
#else
        for (uint8_t i=0; i<(SPIFIFO_SIZE-1); ++i, buffer+=2)
            *(uint16_t *)buffer = SPIFIFO.read();
#endif
    }

#else
    for (; size != 1; --size, ++buffer)
        *buffer = sendByteMore(0xFF);

    *buffer++ = sendByteNoMore(0xFF);
#endif
}

void SPISerialRam::write(const char *buffer, uint32_t address, uint32_t size)
{
    initTransfer(INSTR_WRITE);
    sendAddress(address);

#ifdef VIRTMEM_SPIRAM_USESPIFIFO
    if (size & 1)
    {
        if (size == 1)
        {
            sendByteNoMore(*buffer++);
            return;
        }
        else
            sendByteMore(*buffer++);
        --size;
    }

    const bool usefifo = size > (SPIFIFO_SIZE*2);
    if (usefifo)
    {
        // fill FIFO
        for (uint8_t i=0; i<(SPIFIFO_SIZE-1); ++i, buffer+=2)
        {
#ifdef VIRTMEM_SPIRAM_USENODATASWAP
            SPIFIFO.write16(getWord(buffer[0], buffer[1]), SPI_CONTINUE);
#else
            SPIFIFO.write16(*(uint16_t *)buffer, SPI_CONTINUE);
#endif
        }
        size -= ((SPIFIFO_SIZE-1)*2);
    }

    for (; size != 2; size-=2, buffer+=2)
    {
#ifdef VIRTMEM_SPIRAM_USENODATASWAP
            SPIFIFO.write16(getWord(buffer[0], buffer[1]), SPI_CONTINUE);
#else
            SPIFIFO.write16(*(uint16_t *)buffer, SPI_CONTINUE);
#endif
        SPIFIFO.read();
    }

#ifdef VIRTMEM_SPIRAM_USENODATASWAP
    SPIFIFO.write16(getWord(buffer[0], buffer[1]), 0);
#else
    SPIFIFO.write16(*(uint16_t *)buffer, 0);
#endif
    SPIFIFO.read();

    if (usefifo)
    {
        // purge FIFO
        for (uint8_t i=0; i<(SPIFIFO_SIZE-1); ++i)
            SPIFIFO.read();
    }

#else
    for (; size != 1; --size, ++buffer)
        sendByteMore(*buffer);

    sendByteNoMore(*buffer);
#endif
}

}
