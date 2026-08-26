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

static uint8_t gu08TesterCnt = 0;
//static uint8_t gu08TesterPerLun[MAX_TESTER_LUN];
// P1-2 + P1-3 root-cure: initialize TesterMap with UCHAR_MAX (0xFF) so that
// "empty slot" is correctly represented even on first EnumSm3350 call.
// (Previously zero-initialized static storage meant the `== UCHAR_MAX`
// write-gate in EnumSm3350 was NEVER traversed, and the map never built.)
static uint8_t gu08TesterMap[MAX_DEVICE_CNT] = {
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF
};

void ClearDeviceInfos(void)
{
    // Free any previously allocated interface detail structures to avoid leaks
    for (int i = 0; i < MAX_DEVICE_CNT; ++i)
    {
        if (gstDeviceInfo[i].pDetailData != nullptr)
        {
            // GlobalFree returns NULL on success; ignore return value
            GlobalFree(gstDeviceInfo[i].pDetailData);
            gstDeviceInfo[i].pDetailData = nullptr;
        }
        // clear other fields
        gstDeviceInfo[i].DiskNumber = 0;
        gstDeviceInfo[i].szPhyDrivePath[0] = '\0';
        gstDeviceInfo[i].szDriveName[0] = '\0';
        ZeroMemory(&gstDeviceInfo[i].sdn, sizeof(gstDeviceInfo[i].sdn));
    }
    // P1-3 root-cure: always reset the physical->tester map to all-UCHAR_MAX
    // so subsequent EnumSm3350 passes can re-populate it using the same
    // `== UCHAR_MAX` write-gate they already check.
    memset(gu08TesterMap, UCHAR_MAX, sizeof gu08TesterMap);
    gu08TesterCnt = 0;
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

    // P2-6 fix 1/3: loop variable `i` changed from size_t → UCHAR.
    // Before: size_t triggered C4267 ("size_t → UCHAR potential data loss") when
    //         implicitly converted to UCHAR by getInstance(i) and static_cast<UCHAR>(i).
    // After : i is already a UCHAR. gu08DeviceCnt is guaranteed <= MAX_DEVICE_CNT (16)
    //         by the USB enumeration, so UCHAR overflow is impossible (user confirmed
    //         the platform will never expand past 256 devices).
    for (UCHAR i = 0; i < gu08DeviceCnt; i++)
    {
        CSparkSm3350Util& sm3350 = getInstance(i);

        // P2-3 fix 1/2: pre-clear the UfsReadPortInfo output byte BEFORE this
        // iteration so a successful slot N can never leak its port ID into a
        // subsequent failing slot N+1 that skips the actual UfsReadPortInfo call.
        // (UfsReadPortInfo only overwrites pPortInfo on the SUCCESS path, so
        //  previous bytes survive when the function fails and returns early.)
        //
        // P2-6 fix 2/3: use (CHAR)-1 (NOT (CHAR)UCHAR_MAX nor bare UCHAR_MAX).
        //
        // Why not (CHAR)UCHAR_MAX? → VC /W4 raises C4310 "cast truncates
        // constant value" because UCHAR_MAX=0xFF lies outside the signed-char
        // range [-128..127] even though the bit-pattern is identical.
        // Why not UCHAR_MAX bare? → VC raises C4309 "truncation of constant
        // value" (the original P2-6 part we're fixing here).
        //
        // (CHAR)-1 → in two's complement signed-8bit, -1 has bit pattern
        // 0xFF, which is byte-for-byte identical to UCHAR_MAX. Runtime
        // behaviour is unchanged (FillMemory() below fills gu08TesterMap with
        // 0xFF too, so the two clearing patterns remain perfectly in sync).
        pPortInfo[0x212] = (CHAR)-1;

        // P2-3 fix 2/2: check return values of both DeviceSelect and
        // UfsReadPortInfo. Previously both were silently discarded, causing
        // two failure modes:
        //   (a) DeviceSelect(i) failed but UfsReadPortInfo still ran against
        //       the PREVIOUSLY selected physical slot, so gu08TesterMap[i] got
        //       a cross-slotted ID;
        //   (b) UfsReadPortInfo failed and left stale pPortInfo bytes from the
        //       previous loop iteration, which caused ALL subsequent slots to
        //       inherit that stale ID (duplicate tester IDs -> cross-slot routing).
        int ds = sm3350.DeviceSelect(static_cast<UCHAR>(i));
        if (ds != ERROR_SUCCESS) continue;
        int rp = sm3350.UfsReadPortInfo(pPortInfo);
        if (rp != ERROR_SUCCESS) continue;

        UCHAR u08Id;
        //ReadPortID (buffer has been freshly populated by the successful call above)
        u08Id = pPortInfo[0x212];
        if (u08Id < MAX_DEVICE_CNT)
        {
            if (gu08TesterMap[i] == UCHAR_MAX)
            {
                // P2-5 fix: TesterId uniqueness check.
                // If two phys slots both return the same tester-id (jumper
                // mis-wiring / UfsWritePortInfo duplicate write), a naive write
                // of gu08TesterMap[] stores the id twice → GetPhysicalIndex()
                // first-fit walk always returns the 1st slot, so the 2nd slot's
                // tasks silently run on the 1st device: serial numbers, ISPs
                // and CIDs get overwritten, silently destroying production
                // traceability.
                //
                // Resolution: linear scan [0, i) for any prior slot that
                // already carries this id. If found, leave gu08TesterMap[i] =
                // UCHAR_MAX and do NOT increment gu08TesterCnt. This causes
                // the duplicate physical slot to appear unmapped → it simply
                // won't be exposed to the UI / task pipeline, so zero
                // cross-slot corruption happens.
                bool duplicate = false;
                // P2-6 fix 3/3: uniqueness-check loop variable `j` changed from
                // size_t → UCHAR. Before: size_t j implicit-narrowed to UCHAR
                // when used as gu08TesterMap[j] index → C4267 warning.  After:
                // j < i < gu08DeviceCnt <= 16 < UCHAR_MAX, so no overflow.
                for (UCHAR j = 0; j < i; j++)
                {
                    if (gu08TesterMap[j] == u08Id)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate)
                {
                    // Optional future work: log via SparkLog("WARN: duplicate
                    // tester-id %u at phys=%u, first seen at phys=%u\n"), or
                    // SetLastError(ERROR_DUPLICATE_TAG) so the caller can
                    // surface a warning to the operator (jumper probably wrong).
                    continue;
                }
                gu08TesterMap[i] = u08Id;
                gu08TesterCnt++;
            }
        }
    }

    return gu08TesterCnt;
}

int spark::sm3350::CSparkSm3350Util::GetDevicePath(PhyIndex idx, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned) noexcept
{
    // P1-2/P2-1 root-cure:
    //  * fail fast if caller forgot to provide lpBytesReturned (common when quering size first)
    //  * reject invalid/untested physical indices; idx.value() is already < MAX_DEVICE_CNT by construction
    //  * reject off-by-one: idx.value() MUST be strictly less than gu08DeviceCnt (was `idx > gu08DeviceCnt`
    //    which allows idx==gu08DeviceCnt to pass through)
    //  * guard against gstDeviceInfo[i].pDetailData being null
    if (lpBytesReturned == nullptr)
    {
        return ERROR_INVALID_PARAMETER;
    }
    *lpBytesReturned = 0;
    if (!idx.IsValid())
    {
        return ERROR_INVALID_PARAMETER;
    }
    UCHAR i = idx.value();
    if (i >= gu08DeviceCnt)
    {
#if TOOLSET_VER > 141
        return ERROR_NO_SUCH_DEVICE;
#else
        return 433L;
#endif // TOOLSET_VER
    }
    if (gstDeviceInfo[i].pDetailData == nullptr)
    {
        return ERROR_NO_SUCH_DEVICE;
    }
    const char* devPath = gstDeviceInfo[i].pDetailData->DevicePath;
    if (devPath == nullptr)
    {
        return ERROR_NO_SUCH_DEVICE;
    }
    DWORD len = (DWORD)(strlen(devPath) + 1);
    if (lpOutBuffer == nullptr)
    {
        *lpBytesReturned = len;
        return ERROR_SUCCESS;
    }
    if (len > nOutBufferSize)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memcpy((char*)lpOutBuffer, devPath, len);
    *lpBytesReturned = len;
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

// Primary (root-cure) implementation #1: physical slot -> PST_DEVICE_INFO.
// Fast O(1) path. Preferred for stage code + legacy Scan loop iterators.
PST_DEVICE_INFO spark::sm3350::CSparkSm3350Util::GetDeviceInfo(PhyIndex phys) noexcept
{
    if (!phys.IsValid()) return nullptr;
    UCHAR i = phys.value();
    if (i >= gu08DeviceCnt) return nullptr;   // prevent reading uninitialized gstDeviceInfo[i]
    return &gstDeviceInfo[i];
}

// Primary (root-cure) implementation #2: tester-id -> PST_DEVICE_INFO.
// P2-4 fix 2/3: previously this function did `gu08TesterMap[id.value()]` directly,
// treating the tester-id argument as a PHYSICAL index. It now performs a proper
// reverse lookup (tester -> phys) via GetPhysicalIndex, then delegates to the
// PhyIndex overload above.
PST_DEVICE_INFO spark::sm3350::CSparkSm3350Util::GetDeviceInfo(TesterId id) noexcept
{
    PhyIndex p = GetPhysicalIndex(id);
    if (!p.IsValid()) return nullptr;
    return GetDeviceInfo(p);
}

// Primary (root-cure) implementation: phys -> tester lookup.
// Input: physical-slot index (range-checked by construction).
// Returns: tester id at that slot, or UCHAR_MAX if empty/unmapped.
UCHAR spark::sm3350::CSparkSm3350Util::GetTesterIndex(PhyIndex phys) noexcept
{
    if (!phys.IsValid()) return UCHAR_MAX;
    return gu08TesterMap[phys.value()];              // already [0,15] — zero OOB
}

// Primary (root-cure) implementation: tester -> physical lookup.
// Returns: a valid PhyIndex (never produces UCHAR_MAX-based OOB at the call site).
spark::sm3350::PhyIndex spark::sm3350::CSparkSm3350Util::GetPhysicalIndex(TesterId tester) noexcept
{
    if (!tester.IsValid()) return PhyIndex::Invalid();
    for (UCHAR i = 0; i < gu08DeviceCnt; ++i)        // by construction i < MAX_DEVICE_CNT, so gu08TesterMap[i] safe
    {
        if (gu08TesterMap[i] == tester.value())
        {
            return PhyIndex::FromTrusted(i);         // i < gu08DeviceCnt <= MAX_DEVICE_CNT
        }
    }
    return PhyIndex::Invalid();
}

int spark::sm3350::CSparkSm3350Util::DeviceSelect(PhyIndex idx) noexcept
{
    if (!idx.IsValid())
    {
#if TOOLSET_VER > 141
        return ERROR_NO_SUCH_DEVICE;
#else
        return 433L;
#endif // TOOLSET_VER
    }
    UCHAR i = idx.value();
    if (i >= gu08DeviceCnt)
    {
#if TOOLSET_VER > 141
        return ERROR_NO_SUCH_DEVICE;
#else
        return 433L;
#endif // TOOLSET_VER
    }
    if (gstDeviceInfo[i].pDetailData == nullptr || gstDeviceInfo[i].pDetailData->DevicePath == nullptr)
    {
#if TOOLSET_VER > 141
        return ERROR_NO_SUCH_DEVICE;
#else
        return 433L;
#endif // TOOLSET_VER
    }
    m_szDevicePath = gstDeviceInfo[i].pDetailData->DevicePath;
    //m_szDevicePath = gstDeviceInfo[idx].szPhyDrivePath;
    TRACE("%s\n", m_szDevicePath);
    return m_sm3350Vcmds.OpenDevice(m_szDevicePath, gu08DriverMode);
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::GetCmdResp()
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    SM3350_CMD_DELAY();
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::EnterH8(PCHAR pData)
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsEnterH8(pData))) return ret;
    } while (0);
    SM3350_CMD_DELAY();
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::ExitH8(PCHAR pData)
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsExitH8(pData))) return ret;
    } while (0);
    SM3350_CMD_DELAY();
	return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::ReadCurrent(PCHAR pData)
{
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadCurrent(pData))) return ret;
    } while (0);
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsWritePortInfo(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsWritePortInfo(pData))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsCheckIsp(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdRead(pData, FLAG_CHECK_ISP,8))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsCheckSram2(PCHAR pData1, PCHAR pData2)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdRead(pData1, FLAG_CHECK_SRAM1,8))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdRead(pData2, FLAG_CHECK_SRAM2,8))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
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
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UFSReadPRV(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadStringDescriptor(pData, 0x04,0x0A))) return ret;
    } while (0);
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UFSReadID(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsWriteBufferUpiu(nullptr, 0x534D494E,0x00,0x00))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsReadBufferUpiu(pData, 0x00000301, 0x00000800, 0x04))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;

    } while (0);
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsReadAging(PCHAR pData, UINT nSectorCnt)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdRead(pData, FLAG_READ_AGING, nSectorCnt))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsRead10(pData, 0x00910000, nSectorCnt,nSectorCnt))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
    } while (0);
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

int spark::sm3350::CSparkSm3350Util::UfsReadQ100(PCHAR pData)
{
    TRACE_FUNC();
    int ret;
    do
    {
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsTestUnitReady(pData, 0))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdStart())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsRead10(pData, 0x00,1,1))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsRead10(pData, FLAG_GET_AECQ_INFO,1,1))) return ret;
         if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsRead10(pData, FLAG_SET_LHTDR_WRITE, 1, 1))) return ret;
        //if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsRead10(pData, FLAG_SET_LHTDR_READ, 1, 1))) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.UfsVcmdEnd())) return ret;
        if (ERROR_SUCCESS != (ret = m_sm3350Vcmds.GetCmdResp())) return ret;
    } while (0);
    SM3350_CMD_DELAY();
    return ERROR_SUCCESS;
}

