#pragma once
#include "VolumeDevice.h"
#include <winioctl.h>
#include "FsStructs.h"

class Fat32Volume : public VolumeDevice
{
public:
    Fat32Volume() = default;
    Fat32Volume(LPCSTR szDevicePath /*= nullptr*/);

    int begin(LPVOID lpvoid) override;
    int format(uint32_t nSectorCount = 0) override;

protected:
    PST_FORMAT_INFO m_pFmt = nullptr;
    //uint32_t m_capacityMB = 0;
    uint32_t m_dataStart = 0;
    uint32_t m_fatSize = 0;
    uint32_t m_fatStart = 0;
    uint32_t m_relativeSectors = 0;
    uint32_t m_sectorCount = 0;
    uint32_t m_totalSectors = 0;
    uint16_t m_reservedSectorCount = 0;
    uint8_t m_partType = 0;
    uint8_t m_sectorsPerCluster = 0;

    ST_MBR m_Mbr = { 0 };
    ST_PBR_FAT m_Pbr = { 0 };
    int writeMbr();
    int makeFat32(uint32_t relativeSectors = 0);
    int initFatDir(uint8_t fatType, uint32_t sectorCount);
    void initPbr();
};