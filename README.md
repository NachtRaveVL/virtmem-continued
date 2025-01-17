# virtmem Continued

Original library: https://github.com/rhelmus/virtmem

This version of the library is in maintenance-only mode. The intention is to continue the legacy of the original library, but made to work for modern compilation systems. No new features outside of that which the community or the original author provide are scheduled. Fixes through community pull requests are most appreciated. Please report bugs to issue tracker. Please no feature requests outside of pull requests that do not change any existing interfaces.

## Introduction
`virtmem-continued` is an Arduino library that allows your project to easily use an external memory source to extend the (limited) amount of available RAM. This library supports several memory resources, for instance, SPI ram (e.g. the `23LC1024` or `23K256` chip from Microchip), an SD card or even a computer connected via a serial connection. The library is made in such a way that managing and using this _virtual memory_ closely resembles working with data from 'normal' memory.

## Features
* Extend the available memory with kilobytes, megabytes or even gigabytes
* Supports SPI RAM (23LC/23K series from Microchip), SD cards and RAM from a computer connected through serial
* Easy C++ interface that closely resembles regular data access
* Memory page system to speed up access to virtual memory
* New memory interfaces can be added easily
* Code is mostly platform independent and can fairly easy be ported to other plaforms (x86 port exists for debugging)
* SDVAlloc now works with standard platform SD library (no more outdated SdFat)
* Now includes an internal port of the serialram library (no separate library include)

## Demonstration
~~~{.cpp}
#include <Arduino.h>
#include <SD.h>
#include <virtmem-continued.h>
#include <alloc/sd_alloc.h>

// Simplify virtmem usage
using namespace virtmem;

// Create virtual a memory allocator that uses SD card (with FAT32 filesystem) as virtual memory pool
// The default memory pool size (1 MB) is used.
SDVAlloc vAlloc;

struct MyStruct { int x, y; };

void setup()
{
    vAlloc.start(); // Always call this to initialize the allocator before using it

    // Allocate a char buffer of 10000 bytes in virtual memory and store the address to a virtual pointer
    VPtr<char, SDVAlloc> str = vAlloc.alloc<char>(10000);

    // Set the first 1000 bytes to 'A'
    memset(str, 'A', 1000);

    // array access
    str[1] = 'B';

    // Own types (structs/classes) also work.
    VPtr<MyStruct, SDVAlloc> ms = vAlloc.alloc<MyStruct>(); // alloc call without parameters: use automatic size deduction
    ms->x = 5;
    ms->y = 15;
}

void loop()
{
    // ...
}
~~~

This Arduino sketch demonstrates how to use a SD card as virtual memory
store. By using a virtual memory pointer wrapper class, using virtual memory
becomes quite close to using data residing in 'normal' memory.

## Manual
The _original_ manual [can be found here](http://rhelmus.github.io/virtmem/index.html).

The _continued_ manual (in Doxygen format) [can be found here](https://github.com/NachtRaveVL/virtmem-continued/blob/master/doc/manual.md)

## Benchmark
Some benchmarking results are shown below. Note that these numbers are generated with very simple,
and possibly not so accurate tests, hence they should only be used as a rough indication.

<table>
    <tr>
        <th>Read / Write speed (kB/s)
        <th align="center">Teensy 3.2 (96 MHz)
        <th align="center">Teensy 3.2 (144 MHz)
        <th align="center">Arduino Uno
    <tr>
        <td>Serial
        <td align="center">500 / 373
        <td align="center">496 / 378
        <td align="center">30 / 20
    <tr>
        <td>SD
        <td align="center">1107 / 98
        <td align="center">1102 / 91
        <td align="center">156 / 44
    <tr>
        <td>SPI RAM
        <td align="center">1887 / 1159
        <td align="center">2083 / 1207
        <td align="center">150 / 118
</table>

Some notes:
- Serial: Virtual means that a USB serial connection is used, which is only limited by USB speeds.
- SD/SPI RAM: measured at maximum SPI speeds. For SPI RAM a 23LCV1024 chip was used.
- More details in [the original manual](http://rhelmus.github.io/virtmem/index.html#bench) or [continued manual (Doxy)](https://github.com/NachtRaveVL/virtmem-continued/blob/master/doc/manual.md).
