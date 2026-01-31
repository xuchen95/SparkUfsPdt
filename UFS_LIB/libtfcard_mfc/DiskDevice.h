#pragma once
#include "BlockDevice.h"
#include "FsStructs.h"

class DiskDevice : public BlockDevice
{
public:
    DiskDevice(LPCSTR szDevicePath = nullptr);
    virtual int open(LPCSTR szDevicePath);
    virtual int close();
};

