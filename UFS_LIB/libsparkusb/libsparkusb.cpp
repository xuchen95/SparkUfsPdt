// libsparkusb.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "libsparkusb.h"
#include <winioctl.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <fstream>
#include "SM2706Vcmd.h"
#include "Metorage_Struct.h"
#include "SDMMC.h"
#include "Card.h"
#include "SmiDriverCmds.h"

using namespace std;
using namespace spark::file;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment (lib, "Setupapi.lib")

static uint8_t gu08DriverMode = 0;
static uint8_t gu08DeviceCnt = 0;
static uint64_t gu64DeviceBmp = 0;
static uint8_t gu08DeviceSel = 0;
static ST_DEVICE_INFO gstDeviceInfo[MAX_DEVICE_CNT];

static uint8_t gu08TesterCnt;
//static uint8_t gu08TesterPerLun[MAX_TESTER_LUN];
static uint8_t gu08TesterMap[MAX_DEVICE_CNT];

void ClearDeviceInfos(void)
{
    gu64DeviceBmp = gu08DeviceCnt = gu08DeviceSel = 0;
}

spark::CSparkUsbUtil::CSparkUsbUtil(void)
{
}

int spark::CSparkUsbUtil::EnumVolumeDevices(BOOL (*lpfn)(PSTORAGE_DEVICE_DESCRIPTOR) /*= nullptr*/,
        UINT nDriveType /*= DRIVE_REMOVABLE*/)
{
    HANDLE hVolume, hVolNoBs;
    CHAR szVolumeName[MAX_PATH + 1] = "";
    CHAR szVolNameNoBSlash[MAX_PATH + 1] = "";

    ClearDeviceInfos();

    hVolume = FindFirstVolume(szVolumeName, sizeof(szVolumeName));
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        return ERROR_NO_MORE_FILES;
    }

    do
    {
        if (nDriveType == GetDriveType(szVolumeName))
        {
            strcpy_s(szVolNameNoBSlash, sizeof(szVolNameNoBSlash), szVolumeName);
            szVolNameNoBSlash[strlen(szVolNameNoBSlash) - 1] = '\0';

            TRACE("%s\n", szVolumeName);
            TRACE("%s\n", szVolNameNoBSlash);

            hVolNoBs = INVALID_HANDLE_VALUE;
            hVolNoBs = CreateFile(szVolNameNoBSlash,
                                  FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, 0);
            if (hVolNoBs == INVALID_HANDLE_VALUE)
            {
                TRACE("create handle fail: 0x%08x\n", GetLastError());
            }
            else
            {
                STORAGE_PROPERTY_QUERY spq;
                PSTORAGE_DEVICE_DESCRIPTOR psdd;
                BYTE byBuffer[1024];
                DWORD cbBytesReturned;

                psdd = (PSTORAGE_DEVICE_DESCRIPTOR)byBuffer;
                spq.PropertyId = StorageDeviceProperty;
                spq.QueryType = PropertyStandardQuery;
                spq.AdditionalParameters[0] = 0;

                if (DeviceIoControl(hVolNoBs,
                                    IOCTL_STORAGE_QUERY_PROPERTY,   // operation to perform
                                    &spq, sizeof(spq),              // input buffer
                                    &byBuffer, sizeof(byBuffer),    // output buffer
                                    &cbBytesReturned,               // # bytes returned
                                    (LPOVERLAPPED)NULL))            // synchronous I/O
                {
                    // check if VID, PID match target device
                    if ((lpfn != nullptr) && lpfn(psdd))
                    {
                        STORAGE_DEVICE_NUMBER sdn;
                        if (DeviceIoControl(hVolNoBs,                           // device to be queried
                                            IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                            NULL, 0,                            // no input buffer
                                            (LPVOID)&sdn, sizeof(sdn),          // output buffer
                                            &cbBytesReturned,                   // # bytes returned
                                            (LPOVERLAPPED)NULL))                // synchronous I/O
                        {
                            TRACE("DeviceType: %d, DeviceNumber: %d, PartitionNumber: %d\n",
                                  sdn.DeviceType, sdn.DeviceNumber, sdn.PartitionNumber);

                            if (sdn.DeviceType == FILE_DEVICE_DISK)
                            {
                                if (ERROR_SUCCESS == EnumDiskDevicePath(&sdn))
                                {
                                    VOLUME_DISK_EXTENTS vde;
                                    if (DeviceIoControl(hVolNoBs,
                                                        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                                        nullptr, 0,
                                                        &vde, sizeof(vde),
                                                        &cbBytesReturned,
                                                        nullptr))
                                    {
                                        sprintf_s(gstDeviceInfo[gu08DeviceCnt].szPhyDrivePath,
                                                  "\\\\.\\PhysicalDrive%d",
                                                  vde.Extents[0].DiskNumber);
                                        TRACE("%s\n", gstDeviceInfo[gu08DeviceCnt].szPhyDrivePath);
                                        gstDeviceInfo[gu08DeviceCnt].DiskNumber = vde.Extents[0].DiskNumber;
                                    }

                                    CHAR szDriveName[MAX_PATH];
                                    if (GetVolumePathNamesForVolumeName(szVolumeName,
                                                                        szDriveName,
                                                                        sizeof(szDriveName),
                                                                        &cbBytesReturned))
                                    {
                                        ZeroMemory(gstDeviceInfo[gu08DeviceCnt].szDriveName,
                                                   sizeof(gstDeviceInfo[gu08DeviceCnt].szDriveName));
                                        if (strlen(szDriveName))
                                        {
                                            TRACE("Driver: %s\n", szDriveName, strlen(szDriveName));
                                            // remove SM3350 driver letter
                                            if (FALSE == DeleteVolumeMountPoint(szDriveName))
                                            {
                                                memcpy(gstDeviceInfo[gu08DeviceCnt].szDriveName,
                                                       szDriveName, cbBytesReturned);
                                            }
                                        }
                                    }

                                    gu08DeviceCnt++;
                                }
                            }
                        }
                    }
                }

                CloseHandle(hVolNoBs);
            }
        }
    } while (FindNextVolume(hVolume, szVolumeName, sizeof(szVolumeName)));
    FindVolumeClose(hVolume);

    return gu08DeviceCnt;
}

BOOL spark::CSparkUsbUtil::EnumDevicePath(LPCGUID pguid, PSTORAGE_DEVICE_NUMBER psdn /*= nullptr*/)
{
    HDEVINFO hIntDevInfo;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;
    BOOL bFound = FALSE;
    DWORD dwError = ERROR_NO_MORE_ITEMS;

    hIntDevInfo = SetupDiGetClassDevs(pguid, NULL, NULL, (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));

    if (hIntDevInfo != INVALID_HANDLE_VALUE)
    {
        for (DWORD dwIndex = 0; ; dwIndex++)
        {
            SP_DEVICE_INTERFACE_DATA interfaceData;
            SP_DEVINFO_DATA deviceInfoData;
            DWORD dwRequiredSize;

            ZeroMemory(&interfaceData, sizeof(interfaceData));
            interfaceData.cbSize = sizeof(interfaceData);
            if (!SetupDiEnumDeviceInterfaces(hIntDevInfo, NULL, pguid, dwIndex, &interfaceData))
            {
                dwError = GetLastError();
                if (dwError == ERROR_NO_MORE_ITEMS) // no more devices
                {
                    break;
                }
            }

            dwRequiredSize = 0;
            if (!SetupDiGetDeviceInterfaceDetail(hIntDevInfo, &interfaceData,
                                                 NULL, 0, &dwRequiredSize, NULL) &&
                GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                continue;
            }

            if (pInterfaceDetailData)
            {
                pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalFree(pInterfaceDetailData);
            }
            pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GPTR, dwRequiredSize);
            if (pInterfaceDetailData == nullptr)
            {
                dwError = ERROR_ALLOTTED_SPACE_EXCEEDED;
                break;
            }
            pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            ZeroMemory(&deviceInfoData, sizeof(deviceInfoData));
            deviceInfoData.cbSize = sizeof(deviceInfoData);
            if (!SetupDiGetDeviceInterfaceDetail(hIntDevInfo, &interfaceData,
                                                 pInterfaceDetailData, dwRequiredSize,
                                                 &dwRequiredSize, &deviceInfoData))
            {
                continue;
            }

            hDevice = CreateFile(pInterfaceDetailData->DevicePath,
                                 0,                                     // no access to the drive
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,    // share mode
                                 nullptr,                               // default security attributes
                                 OPEN_EXISTING,                         // disposition
                                 0,                                     // file attributes
                                 nullptr);                              // do not copy file attributes

            if (hDevice != INVALID_HANDLE_VALUE)
            {
                TRACE("Device Path: %s\n", pInterfaceDetailData->DevicePath);
                if (psdn != nullptr)
                {
                    STORAGE_DEVICE_NUMBER sdn;
                    DWORD cbBytesReturned;
                    if (DeviceIoControl(hDevice,                            // device to be queried
                                        IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                        nullptr, 0,                         // no input buffer
                                        (LPVOID)&sdn, sizeof(sdn),          // output buffer
                                        &cbBytesReturned,                   // # bytes returned
                                        (LPOVERLAPPED)nullptr))             // synchronous I/O
                    {
                        if (sdn.DeviceType == psdn->DeviceType &&
                            sdn.DeviceNumber == psdn->DeviceNumber)
                        {
                            gu64DeviceBmp |= (1ULL << sdn.DeviceNumber);
                            gstDeviceInfo[gu08DeviceCnt].sdn = *psdn;
                            gstDeviceInfo[gu08DeviceCnt].pDetailData = pInterfaceDetailData;

                            bFound = TRUE;
                            break;
                        }
                    }
                }
                else
                {
                    gstDeviceInfo[gu08DeviceCnt++].pDetailData = pInterfaceDetailData;
                    pInterfaceDetailData = nullptr;
                    bFound = TRUE;
                }

                CloseHandle(hDevice);
                hDevice = INVALID_HANDLE_VALUE;
            }
        }

        if ((FALSE == bFound) && (pInterfaceDetailData != nullptr))
        {
            pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalFree(pInterfaceDetailData);
        }
        if (hDevice != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hDevice);
        }
        SetupDiDestroyDeviceInfoList(hIntDevInfo);
    }

    return bFound;
}

int spark::CSparkUsbUtil::EnumDiskDevicePath(PSTORAGE_DEVICE_NUMBER psdn)
{
    if (EnumDevicePath((LPCGUID)&GUID_DEVINTERFACE_DISK, psdn))
    {
        return ERROR_SUCCESS;
    }
    else
    {
#if TOOLSET_VER > 141
        return ERROR_NO_SUCH_DEVICE;
#else
        return 433L;
#endif /* TOOLSET_VER */
    }
}

spark::sm3350::CSparkSm3350Util::CSparkSm3350Util()
{
}

spark::sm3350::CSparkSm3350Util::~CSparkSm3350Util()
{
}

#define SM3350_VID       "SMI"
#define SM3350_PID       "USB-UFS BRIDGE"
BOOL EnumSm3350CallbackFun2(PSTORAGE_DEVICE_DESCRIPTOR psdd)
{
    PBYTE pVendorId, pProductId, pProductRevision;
    int off;

    off = offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties);
    pVendorId = &psdd->RawDeviceProperties[psdd->VendorIdOffset - off];
    pProductId = &psdd->RawDeviceProperties[psdd->ProductIdOffset - off];
    pProductRevision = &psdd->RawDeviceProperties[psdd->ProductRevisionOffset - off];
    TRACE("VendorId: %hs\n", pVendorId);
    TRACE("ProductId: %hs\n", pProductId);
    TRACE("ProductRevision: %hs\n", pProductRevision);

    if (!memcmp(SM3350_VID, pVendorId, strlen(SM3350_VID)) &&
        !memcmp(SM3350_PID, pProductId, strlen(SM3350_PID)))
    {
        return TRUE;
    }

    return FALSE;
}

int spark::sm3350::CSparkSm3350Util::EnumSm3350(int nDriverMode /*= 0*/)
{
    CHAR pPortInfo[1024];
    gu08DriverMode = nDriverMode;
    ClearDeviceInfos();

    if (nDriverMode)
    {
        (void)CSparkUsbUtil::EnumDevicePath(&GUID_DEVINTERFACE_SMI_SM3350);
    }
    else
    {
        (void)(CSparkUsbUtil::EnumVolumeDevices(EnumSm3350CallbackFun2));
    }

    gu08TesterCnt = 0;
    FillMemory(gu08TesterMap, sizeof(gu08TesterMap), 0xFF);
    //ZeroMemory(gu08TesterPerLun, sizeof(gu08TesterPerLun));

    for (size_t i = 0; i < gu08DeviceCnt; i++)
    {
        
        CSparkSm3350Util& sm3350 = getInstance(i);

        sm3350.DeviceSelect(i);
        UCHAR u08Id;
		sm3350.UfsReadPortInfo(pPortInfo);
        //ReadPortID
        u08Id = pPortInfo[0x212];
        if (u08Id < MAX_DEVICE_CNT)
        {
            if (gu08TesterMap[i] == UCHAR_MAX)
            {
                gu08TesterMap[i] = u08Id;
                gu08TesterCnt++;
            }
        }
    }

    return gu08TesterCnt;
}

int spark::sm3350::CSparkSm3350Util::GetDevicePath(unsigned char idx, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned)
{
    if (idx > gu08DeviceCnt)
    {
#if TOOLSET_VER > 141
        return ERROR_NO_SUCH_DEVICE;
#else
        return 433L;
#endif // TOOLSET_VER
    }

    if (lpOutBuffer == nullptr)
    {
        *lpBytesReturned = (strlen(gstDeviceInfo[idx].pDetailData->DevicePath) + 1);
    }
    else if (strlen(gstDeviceInfo[idx].pDetailData->DevicePath) > nOutBufferSize)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        memcpy((char*)lpOutBuffer, (char*)gstDeviceInfo[idx].pDetailData->DevicePath, nOutBufferSize);
    }

    return ERROR_SUCCESS;
}

PST_DEVICE_INFO spark::sm3350::CSparkSm3350Util::GetDeviceInfo()
{
    if (gu08DeviceCnt)
    {
        return gstDeviceInfo;
    }
    else
    {
        return nullptr;
    }
}

UCHAR spark::sm3350::CSparkSm3350Util::GetTesterIndex(UCHAR id)
{
    return gu08TesterMap[id];
}

UCHAR spark::sm3350::CSparkSm3350Util::GetPhysicalIndex(UCHAR testerIdx)
{
    for (UCHAR i = 0; i < gu08DeviceCnt; i++)
    {
        if (gu08TesterMap[i] == testerIdx)
        {
            return i;
        }
    }
    return UCHAR_MAX;
}


PST_DEVICE_INFO spark::sm3350::CSparkSm3350Util::GetDeviceInfo(UCHAR id)
{
    if (gu08TesterMap[id] != UCHAR_MAX)
    {
        return &gstDeviceInfo[gu08TesterMap[id]];
    }

    return nullptr;
}


int spark::sm3350::CSparkSm3350Util::DeviceSelect(UCHAR idx /*= 0*/)
{
    if (idx >= gu08DeviceCnt)
    {
#if TOOLSET_VER > 141
        return ERROR_NO_SUCH_DEVICE;
#else
        return 433L;
#endif // TOOLSET_VER
    }
    else
    {
        m_szDevicePath = gstDeviceInfo[idx].pDetailData->DevicePath;
        //m_szDevicePath = gstDeviceInfo[idx].szPhyDrivePath;
        TRACE("%s\n", m_szDevicePath);
        return m_sm3350Vcmds.OpenDevice(m_szDevicePath, gu08DriverMode);
    }
}

int spark::sm3350::CSparkSm3350Util::UpiuForceRom(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsUpiuForceRomCodeModeForUfs(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::VccOffForceRom(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVccOffForceRomModeUfs(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsPowerOn(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsPowerOn(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResponse())) return ret;
    } while (0);
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsPowerOff(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsPowerOff(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResponse())) return ret;
    } while (0);
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::GetCmdResp()
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::EnterH8(PCHAR pData)
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsEnterH8(pData))) return ret;
    } while (0);
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::ExitH8(PCHAR pData)
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsExitH8(pData))) return ret;
    } while (0);
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::ReadCurrent(PCHAR pData)
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadCurrent(pData))) return ret;
    } while (0);
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsMpStartMode(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsMpStartMode(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsWrite1024KIspMp(PCHAR pData, UINT nSectorCnt, BOOL bEraseAllBlock)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsWrite1024KIspMp(pData, nSectorCnt, bEraseAllBlock))) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsMpExit(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsMpExit(pData))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;

    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsCardInit(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsCardInit(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsReadPortInfo(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadPortInfo(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsWritePortInfo(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadPortInfo(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsSetSrialNumberString(PCHAR pData)
{
    
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdWrite(pData,FLAG_WRITE_PSN))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsSetManuDate(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdWrite(pData, FLAG_WRITE_MDT))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsCheckIsp(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdRead(pData, FLAG_CHECK_ISP))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsCheckSram2(PCHAR pData1, PCHAR pData2)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdRead(pData1, FLAG_CHECK_SRAM1))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdRead(pData2, FLAG_CHECK_SRAM2))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsWriteSramMp(PCHAR pData, UINT nSectorCnt)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsWriteSramMp(pData, nSectorCnt))) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsReadSramResult(PCHAR pData, UINT nSectorCnt)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsRead10(pData,1,0,8))) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsReadCidInfo(PCHAR pData, UINT nSectorCnt)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadCid(pData, nSectorCnt))) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsGetGeometry(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    DWORD dwSpecLen = 0x00000057;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadGeometryDescriptor(pData, dwSpecLen))) return ret;
    } while (0);
    return ERROR_SUCCESS;
}

