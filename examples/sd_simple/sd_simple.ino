/*
 * Minimal example showing how to use the SD virtual memory allocator
 * (SDVAlloc/SDVAllocP). This allocator uses a file on a FAT32 formatted SD card as RAM.
 *
 * Requirements:
 *  - a FAT32 formatted SD card (SDHC recommended)
 *  - a connection to the SD card via SPI
 */


#include <Arduino.h>
#include <virtmem-continued.h>
#include <alloc/sd_alloc.h>

// pull in complete virtmem namespace
using namespace virtmem;

// configuration for SD
const int chipSelect = SD_CHIP_SELECT_PIN;
const uint32_t poolSize = 1024l * 32l; // the size of the virtual memory pool (in bytes)
const int spiSpeed = SD_SPI_SPEED;

SDVAlloc sdvAlloc(poolSize, chipSelect, spiSpeed);

void setup()
{
    // uncomment if using the ethernet shield
    // pinMode(10, OUTPUT); digitalWrite(10, HIGH);

    while (!Serial)
        ; // wait for serial to come up

    Serial.begin(115200);

    sdvAlloc.start();

    delay(3000); // add some delay so the user can connect with a serial terminal
}

void loop()
{
    // allocate some integer on virtual memory
    VPtr<int, SDVAlloc> vpi = sdvAlloc.alloc<int>();

    *vpi = 42; // assign some value, just like a regular pointer!
    Serial.print("*vpi = "); Serial.println(*vpi);

    sdvAlloc.free(vpi); // And free the virtual memory

    delay(1000); // keep doing this with 1 second pauses inbetween...
}
