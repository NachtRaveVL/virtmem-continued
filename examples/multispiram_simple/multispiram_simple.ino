/*
 * Minimal example showing how to use the multi SPI RAM virtual memory allocator
 * (MultiSPIRAMVAllocP). The 23LC/23K series from Microchip are supported. In this example
 * two chips are used. The total memory of both chips will be combined by the allocator.
 *
 * Requirements:
 *  - two SRAM chips should be properly connected with SPI (the CS pins are configured below)
 */


#include <Arduino.h>
#include <virtmem-continued.h>
#include <alloc/spiram_alloc.h>

// pull in complete virtmem namespace
using namespace virtmem;

// configuration for two 23LC1024 chips (128 kByte in size), CS connected to pins 9 & 10
SPISerialRamConfig scfg[2] =
{
    // format: <size of the chip>, <CS pin>, <SPI speed>
    { 1024l * 128l, 9, SRAM_SPI_SPEED },
    { 1024l * 128l, 10, SRAM_SPI_SPEED }
};

typedef MultiSPIRAMVAllocP<scfg, 2> Alloc; // shortcut
Alloc vAlloc;

void setup()
{
    while (!Serial)
        ; // wait for serial to come up

    Serial.begin(115200);
    vAlloc.start();

    delay(3000); // add some delay so the user can connect with a serial terminal
}

void loop()
{
    // allocate some integer on virtual memory
    VPtr<int, Alloc> vpi = vAlloc.alloc<int>();

    *vpi = 42; // assign some value, just like a regular pointer!
    Serial.print("*vpi = "); Serial.println(*vpi);

    vAlloc.free(vpi); // And free the virtual memory

    delay(1000); // keep doing this with 1 second pauses inbetween...
}
