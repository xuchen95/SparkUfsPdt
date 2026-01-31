#include "pch.h"
#include "DiskDevice.h"

DiskDevice::DiskDevice(LPCSTR szDevicePath /*= nullptr*/)
{

}

int DiskDevice::open(LPCSTR szDevicePath)
{
    return ERROR_SUCCESS;
}

int DiskDevice::close()
{
    return ERROR_SUCCESS;
}
