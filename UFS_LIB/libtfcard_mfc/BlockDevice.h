#pragma once

#include <cstdint>

#define SECTOR2BYTE(x)              ((x) << 9)
#define BYTE2SECTOR(x)              ((x + 511) >> 9)

class BlockDevice
{
public:
    virtual ~BlockDevice() {};

    virtual bool isBusy() { return false; };
    virtual uint32_t sectorCount() = 0;
    virtual int readSectors(uint32_t sector, uint8_t* dest, size_t ns = 1) = 0;
    virtual int writeSectors(uint32_t sector, const uint8_t* src, size_t ns = 1) = 0;
    virtual int sync() = 0;
};

