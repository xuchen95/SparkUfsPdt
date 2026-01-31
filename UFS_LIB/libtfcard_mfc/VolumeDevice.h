#pragma once
#include "DiskDevice.h"
#include "FormatInfo.h"
class VolumeDevice : public DiskDevice
{
public:
    VolumeDevice(LPCSTR szDevicePath = nullptr);
    virtual ~VolumeDevice();
    uint32_t sectorCount() override;
    int lockUnLock(bool bLock = true);
    int readSectors(uint32_t sector, uint8_t* dest, size_t ns = 1) override;
    int writeSectors(uint32_t sector, const uint8_t* src, size_t ns = 1) override;
    int sync() override;
    virtual int begin(LPVOID) = 0;
    virtual int format(uint32_t nSectorCount = 0) = 0;
    int close() override;

    HANDLE Handle() const { return m_Handle; }
    void Handle(HANDLE val) { m_Handle = val; }
protected:
    LPCSTR m_szDevicePath = nullptr;
    HANDLE m_Handle = INVALID_HANDLE_VALUE;
};

