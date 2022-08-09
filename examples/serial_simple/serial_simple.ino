/*
 * Minimal example showing how to use the serial virtual memory allocator
 * (SerialVAlloc/SerialVAllocP). This allocator uses RAM from an external device
 * connected through serial.
 *
 * The only requirement is that a so called 'RAM host' (for instance a PC) is
 * connected via the default serial port (Serial). The RAM host should run the
 * 'extras/serial_host.py' Python script.
 */


#include <Arduino.h>
#include <virtmem-continued.h>
#include <alloc/serial_alloc.h>

// pull in complete virtmem namespace
using namespace virtmem;

const uint32_t poolSize = 1024l * 32l; // the size of the virtual memory pool (in bytes)

SerialVAlloc vAlloc(poolSize); // default settings: use Serial with 115200 baudrate

// Example for using Serial1 with 9600 baudrate
//SerialVAllocP<typeof(Serial1)> vAlloc(poolSize, 9600, Serial1);

void setup()
{
    while (!Serial)
        ; // wait for serial to come up

    vAlloc.start();
}

void loop()
{
    // allocate some integer on virtual memory
    VPtr<int, SerialVAlloc> vpi = vAlloc.alloc<int>();

    *vpi = 42; // assign some value, just like a regular pointer!
    Serial.print("*vpi = "); Serial.println(*vpi);

    vAlloc.free(vpi); // And free the virtual memory

    delay(1000); // keep doing this with 1 second pauses inbetween...
}
