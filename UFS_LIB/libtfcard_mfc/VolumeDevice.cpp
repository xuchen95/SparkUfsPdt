#include "pch.h"
#include "VolumeDevice.h"
#include <winioctl.h>

VolumeDevice::VolumeDevice(LPCSTR szDevicePath /*= nullptr*/)
{
    if (szDevicePath != nullptr)
    {
        m_szDevicePath = szDevicePath;

        if (szDevicePath[1] == ':')
        {
            CHAR szDrivePath[] = "\\\\.\\C:";
            szDrivePath[4] = szDevicePath[0];

            m_Handle = CreateFile(szDrivePath,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);
        }
        else
        {
            m_Handle = CreateFile(szDevicePath,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);
        }
    }
}

VolumeDevice::~VolumeDevice()
{
    if ((m_szDevicePath != nullptr) && (m_Handle != INVALID_HANDLE_VALUE))
    {
        CloseHandle(m_Handle);
    }
}

uint32_t VolumeDevice::sectorCount()
{
    return 0;
}

int VolumeDevice::lockUnLock(bool bLock /*= true*/)
{
    DWORD dwBytesReturn, dwCtrlCode;

    if (bLock)
    {
        dwCtrlCode = FSCTL_LOCK_VOLUME;
    }
    else
    {
        dwCtrlCode = FSCTL_UNLOCK_VOLUME;
    }

    if (!DeviceIoControl(m_Handle, dwCtrlCode,
                         NULL, 0,
                         NULL, 0,
                         &dwBytesReturn, NULL))
    {
        TRACE("%s\nerror code: %08x\n", __FUNCTION__, GetLastError());
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

int VolumeDevice::readSectors(uint32_t sector, uint8_t* dest, size_t ns /*= 1*/)
{
    DWORD dwBytesReturn;

    if (m_Handle == INVALID_HANDLE_VALUE)
    {
        return ERROR_INVALID_HANDLE;
    }

    SetFilePointer(m_Handle, SECTOR2BYTE(sector), NULL, FILE_BEGIN);
    if (ReadFile(m_Handle, dest, SECTOR2BYTE(ns), &dwBytesReturn, NULL))
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return GetLastError();
    }
}

int VolumeDevice::writeSectors(uint32_t sector, const uint8_t* src, size_t ns /*= 1*/)
{
    DWORD dwBytesReturn;

    if (m_Handle == INVALID_HANDLE_VALUE)
    {
        return ERROR_INVALID_HANDLE;
    }

    SetFilePointer(m_Handle, SECTOR2BYTE(sector), NULL, FILE_BEGIN);
    if (WriteFile(m_Handle, src, SECTOR2BYTE(ns), &dwBytesReturn, NULL))
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return GetLastError();
    }
}

int VolumeDevice::sync()
{
    DWORD dwBytesReturn;

    if (!DeviceIoControl(m_Handle, IOCTL_DISK_UPDATE_PROPERTIES,
        NULL, 0,
        NULL, 0,
        &dwBytesReturn, NULL))
    {
        TRACE("%s\nerror code: %08x\n", __FUNCTION__, GetLastError());
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

int VolumeDevice::close()
{
    if (CloseHandle(m_Handle))
    {
        m_Handle = INVALID_HANDLE_VALUE;
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_INVALID_HANDLE;
    }
}
