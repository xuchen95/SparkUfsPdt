#pragma once
#include "Fat32Volume.h"
class ExFatVolume :
    public Fat32Volume
{
public:
    ExFatVolume() = default;
    ExFatVolume(LPCSTR szDevicePath /*= nullptr*/);
    uint32_t m_upcaseSector;
    uint32_t m_upcaseChecksum;
    uint32_t m_upcaseSize;
    int format(uint32_t nSectorCount = 0) override;

protected:
    int makeExFat();

    //ST_PBR_EXFAT m_Pbr = { 0 };
    U_PBR_EXFAT m_uPbr = { 0 };
    uint32_t exFatChecksum(uint32_t sum, uint8_t data);
    bool writeUpcase(uint32_t sector);
    bool writeUpcaseUnicode(uint16_t unicode);
    bool writeUpcaseByte(uint8_t b);
    bool syncUpcase();
};

