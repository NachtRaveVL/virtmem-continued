#include <Arduino.h>
#include <virtmem-continued.h>
#include "benchmark.h"

//#define RUN_STATICALLOC
//#define RUN_SPIRAMALLOC
//#define RUN_NATIVE
//#define RUN_SERIALALLOC
#define RUN_SDALLOC

// uncomment to disable a SPI select pin, useful when using ethernet shield
#define DISABLE_SELECTPIN1 7
#define DISABLE_SELECTPIN2 8
#define DISABLE_SELECTPIN3 9
#define DISABLE_SELECTPIN4 10

#define NATIVE_BUFSIZE 1024 * 1
#define NATIVE_REPEATS 100

#define STATICALLOC_POOLSIZE 1024l * 1l + 128l // plus some size overhead
#define STATICALLOC_BUFSIZE 1024l * 127l
#define STATICALLOC_REPEATS 100

#define SPIRAM_POOLSIZE 1024l * 128l
#define SPIRAM_BUFSIZE 1024l * 12l
#define SPIRAM_REPEATS 5
#define SPIRAM_CSPIN 9
#define SPIRAM_SPISPEED SRAM_SPI_SPEED

#define SERIALRAM_POOLSIZE 1024l * 1024l
#define SERIALRAM_BUFSIZE 1024l * 12l
#define SERIALRAM_REPEATS 5

#define SD_POOLSIZE 1024l * 1024l
#define SD_BUFSIZE 1024l * 12l
#define SD_REPEATS 5
#define SD_CSPIN 4
#define SD_SPISPEED SD_SPI_SPEED


#ifdef RUN_STATICALLOC
#include <alloc/static_alloc.h>
StaticVAllocP<STATICALLOC_POOLSIZE> staticAlloc;
#endif

#ifdef RUN_SPIRAMALLOC
#include <SPI.h>
#include <alloc/spiram_alloc.h>
SPIRAMVAlloc spiRamAlloc(SPIRAM_POOLSIZE, true, SPIRAM_CSPIN, SPIRAM_SPISPEED);
#endif

#ifdef RUN_SERIALALLOC
#include <alloc/serial_alloc.h>
SerialVAlloc serialRamAlloc(SERIALRAM_POOLSIZE, /*115200*/1000000);
#endif

#ifdef RUN_SDALLOC
#include <SD.h>
#include <alloc/sd_alloc.h>
SDVAlloc sdRamAlloc(SD_POOLSIZE, SD_CSPIN, SD_SPISPEED);
#endif


#ifdef RUN_NATIVE
void runNativeBenchmark(uint32_t bufsize, uint8_t repeats)
{
    volatile char buf[bufsize];

    printBenchStart(bufsize, repeats);

    const uint32_t time = millis();
    for (uint8_t i=0; i<repeats; ++i)
    {
        for (uint32_t j=0; j<bufsize; ++j)
            buf[j] = (char)j;
    }

    printBenchEnd(millis() - time, bufsize, repeats);
}
#endif

void disableCSPin(uint8_t pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

void setup()
{
#ifdef DISABLE_SELECTPIN1
    disableCSPin(DISABLE_SELECTPIN1);
#endif

#ifdef DISABLE_SELECTPIN2
    disableCSPin(DISABLE_SELECTPIN2);
#endif

#ifdef DISABLE_SELECTPIN3
    disableCSPin(DISABLE_SELECTPIN3);
#endif

#ifdef DISABLE_SELECTPIN4
    disableCSPin(DISABLE_SELECTPIN4);
#endif

#ifndef RUN_SERIALALLOC
    while (!Serial)
        ;

    Serial.begin(115200);

    delay(3000);
#endif
}

void loop()
{
#ifdef RUN_NATIVE
    Serial.println("Running native...\n");
    runNativeBenchmark(NATIVE_BUFSIZE, NATIVE_REPEATS);
    Serial.println("\nDone!");
#endif

#ifdef RUN_STATICALLOC
    Serial.println("Running static allocator...\n");
    runBenchmarks(staticAlloc, STATICALLOC_BUFSIZE, STATICALLOC_REPEATS);
    Serial.println("\nDone!");
#endif

#ifdef RUN_SPIRAMALLOC
    Serial.println("Running SPI ram allocator...\n");
    runBenchmarks(spiRamAlloc, SPIRAM_BUFSIZE, SPIRAM_REPEATS);
    Serial.println("\nDone!");
#endif

#ifdef RUN_SERIALALLOC
    Serial.println("Running serial ram allocator...\n");
    runBenchmarks(serialRamAlloc, SERIALRAM_BUFSIZE, SERIALRAM_REPEATS);
    Serial.println("\nDone!");
#endif

#ifdef RUN_SDALLOC
    Serial.println("Running sd fat allocator...\n");
    runBenchmarks(sdRamAlloc, SD_BUFSIZE, SD_REPEATS);
    Serial.println("\nDone!");
#endif

    delay(2000);
}
