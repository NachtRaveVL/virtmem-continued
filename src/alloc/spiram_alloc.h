#ifndef VIRTMEM_SPIRAM_ALLOC_H
#define VIRTMEM_SPIRAM_ALLOC_H

/**
  * @file
  * @brief This file contains the SPI RAM virtual memory allocator
  */

#include <Arduino.h>
#include "internal/alloc.h"
#include "internal/spiram.h"

namespace virtmem {

#ifndef SRAM_CHIP_SELECT_PIN
#if defined(SRAM_SS_PIN)
#define SRAM_CHIP_SELECT_PIN SRAM_SS_PIN
#else
#define SRAM_CHIP_SELECT_PIN SS
#endif
#endif

#ifndef SRAM_SPI_SPEED
#if defined(F_CPU)
#define SRAM_SPI_SPEED F_CPU
#elif defined(F_BUS)
#define SRAM_SPI_SPEED F_BUS
#elif defined(SPI_FULL_SPEED)
#define SRAM_SPI_SPEED SPI_FULL_SPEED
#else
#define SRAM_SPI_SPEED 50000000
#endif
#endif

/**
 * @brief Virtual memory allocator that uses SPI (serial) RAM (e.g. the 23LC/23K series from Microchip)
 * as memory pool. Interfacing occurs through the internal SPISerialRam library.
 *
 * @tparam Properties Allocator properties, see DefaultAllocProperties
 *
 * @sa @ref bUsing and MultiSPIRAMVAllocP
 */
template <typename Properties=DefaultAllocProperties>
class SPIRAMVAllocP : public VAlloc<Properties, SPIRAMVAllocP<Properties> >
{
    SPISerialRam _spiSRam;

    void doStart(void)
    {
        _spiSRam.begin();
    }

    void doStop(void)
    { }

    void doRead(void *data, VPtrSize offset, VPtrSize size)
    {
//        const uint32_t t = micros();
        _spiSRam.read((char *)data, offset, size);
//        Serial.print("read: "); Serial.print(offset); Serial.print("/"); Serial.print(size); Serial.print("/"); Serial.println(micros() - t);
    }

    void doWrite(const void *data, VPtrSize offset, VPtrSize size)
    {
//        const uint32_t t = micros();
        _spiSRam.write((const char *)data, offset, size);
//        Serial.print("write: "); Serial.print(offset); Serial.print("/"); Serial.print(size); Serial.print("/"); Serial.println(micros() - t);
    }

public:
    /**
     * @brief Constructs (but not initializes) the allocator.
     * @param ps Total amount of bytes of the memory pool (i.e. the size of the SRAM chip)
     * @param cs Chip select (CS) pin connected to the SRAM chip
     * @param sp SPI speed to be used in Hz
     * @sa setPoolSize
     */
    SPIRAMVAllocP(VPtrSize ps, pintype_t cs = SRAM_CHIP_SELECT_PIN, uint32_t sp = SRAM_SPI_SPEED) :
        _spiSRam(ps, cs, sp) { this->setPoolSize(ps); }
    ~SPIRAMVAllocP(void) { doStop(); }
};

typedef SPIRAMVAllocP<> SPIRAMVAlloc; //!< Shortcut to SPIRAMVAllocP with default template arguments

/**
 * @brief This `struct` is used to configure each SRAM chip used by a MultiSPIRAMVAllocP
 * allocator.
 */
struct SPISerialRamConfig
{
    uint32_t size; //!< Amount of bytes this chip can hold
    pintype_t chipSelect; //!< Pin assigned as chip select (CS) for this chip
    uint32_t speed; //!< SPI speed to be used in Hz
};

/**
 * @brief Virtual allocator that uses multiple SRAM chips (e.g. 23LC/23K series from Microchip)
 * as memory pool.
 * 
 * This allocator is similar to SPIRAMVAlloc, but combines multiple SRAM chips as one large memory pool.
 * Interfacing occurs through the internal SPISerialRam library. Every SRAM chip is configured by defining
 * an SPISerialRamConfig array:
 *
 * @code{.cpp}
 * // configuration for two 23LC1024 chips, connected to CS pins 9 and 10.
 * virtmem::SPISerialRamConfig scfg[2] = {
 *      { 1024 * 128, 9, SRAM_SPI_SPEED },
 *      { 1024 * 128, 10, SRAM_SPI_SPEED } };
 *
 * virtmem::MultiSPIRAMVAllocP<scfg, 2> alloc;
 * @endcode
 *
 * @tparam spiChips An array of SPISerialRamConfig that is used to configure each individual SRAM chip.
 * @tparam chipAmount Amount of SRAM chips to be used.
 * @tparam Properties Allocator properties, see DefaultAllocProperties
 * @sa @ref bUsing, SPISerialRamConfig and SPIRAMVAllocP
 *
 */
template <const SPISerialRamConfig *spiChips, size_t chipAmount, typename Properties=DefaultAllocProperties>
class MultiSPIRAMVAllocP : public VAlloc<Properties, MultiSPIRAMVAllocP<spiChips, chipAmount, Properties> >
{
    SPISerialRam _spiSRams[chipAmount];

    void doStart(void)
    {
        for (uint8_t i=0; i<chipAmount; ++i)
            _spiSRams[i].begin(spiChips[i].size, spiChips[i].chipSelect, spiChips[i].speed);
    }

    void doStop(void)
    { }

    void doRead(void *data, VPtrSize offset, VPtrSize size)
    {
//        const uint32_t t = micros();
        VPtrNum startptr = 0;
        for (uint8_t i=0; i<chipAmount; ++i)
        {
            const VPtrNum endptr = startptr + spiChips[i].size;
            if (offset >= startptr && offset < endptr)
            {
                const VPtrNum p = offset - startptr; // address relative in this chip
                const VPtrSize sz = private_utils::minimal(size, spiChips[i].size - p);
                _spiSRams[i].read((char *)data, p, sz);

                if (sz == size)
                    break;

                size -= sz;
                data = (char *)data + sz;
                offset += sz;
            }
            startptr = endptr;
        }
//        Serial.print("read: "); Serial.print(size); Serial.print("/"); Serial.println(micros() - t);
    }

    void doWrite(const void *data, VPtrSize offset, VPtrSize size)
    {
//        const uint32_t t = micros();
        VPtrNum startptr = 0;
        for (uint8_t i=0; i<chipAmount; ++i)
        {
            const VPtrNum endptr = startptr + spiChips[i].size;
            if (offset >= startptr && offset < endptr)
            {
                const VPtrNum p = offset - startptr; // address relative in this chip
                const VPtrSize sz = private_utils::minimal(size, spiChips[i].size - p);
                _spiSRams[i].write((const char *)data, p, sz);

                if (sz == size)
                    break;

                size -= sz;
                data = (const char *)data + sz;
                offset += sz;
            }
            startptr = endptr;
        }
//        Serial.print("write: "); Serial.print(size); Serial.print("/"); Serial.println(micros() - t);
    }

    using BaseVAlloc::setPoolSize;

public:
    /**
     * Constructs the allocator. The pool size is automatically
     * deduced from the chip configurations.
     */
    MultiSPIRAMVAllocP(void)
    {
        uint32_t ps = 0;
        for (uint8_t i=0; i<chipAmount; ++i)
            ps += spiChips[i].size;
        this->setPoolSize(ps);
    }
    ~MultiSPIRAMVAllocP(void) { doStop(); }
};

/**
 * @example spiram_simple.ino
 * This is a simple example sketch showing how to use the SPI RAM allocator.
 *
 * @example multispiram_simple.ino
 * This is a simple example sketch showing how to use the multi SPI RAM allocator.
 */

}

#endif // VIRTMEM_SPIRAM_ALLOC_H
