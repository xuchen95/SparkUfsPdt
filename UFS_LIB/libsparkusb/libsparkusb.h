// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LIBSPARKUSB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LIBSPARKUSB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once
#include "pch.h"
#include <winioctl.h>
#include <SetupAPI.h>
#include "libsm3350.h"
#include "SM2706Vcmd.h"
#if TOOLSET_VER > 141
#include <minwindef.h>
#endif // TOOLSET_VER
#include "StateImp.h"

#ifdef LIBSPARK_EXPORTS
#define LIBSPARK_API __declspec(dllexport)
#else
#define LIBSPARK_API __declspec(dllimport)
#endif

#define TRACE_FUNC()        TRACE("%s\n", __FUNCTION__)
#define SM3350_CMD_DELAY()  Sleep(100)
//#define MAX_TESTER_LUN          (2)
//#define MAX_TESTER_PER_LUN      (8)

static constexpr int MAX_DEVICE_CNT = 16;
static constexpr std::size_t MAX_TINY_CODE_SIZE = 32768;
static constexpr std::size_t MAX_BOOT_INFO_SIZE = 3072;

static constexpr std::size_t UFS_ISP_SIZE = 1024ULL * 512ULL * 2ULL;

static constexpr int UFS_ERASE_ALL_BLOCK = 0;
static constexpr int UFS_ERASE_GOOD_BLOCK = 1;

typedef union
{
    CHAR        int8[512];
    SHORT       int16[256];
    INT         int32[128];
    UCHAR       uint8[512];
    USHORT      uint16[256];
    UINT        uint32[128];
} U_MIX, PU_MIX;

typedef struct
{
    DWORD DiskNumber;
    CHAR szPhyDrivePath[MAX_PATH];
    CHAR szDriveName[8];
    STORAGE_DEVICE_NUMBER sdn;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData;
} ST_DEVICE_INFO, * PST_DEVICE_INFO;

namespace spark
{
    class LIBSPARK_API CSparkUsbUtil
    {
    public:
        CSparkUsbUtil(void);

        /// <summary>
        /// Enum volume devices
        /// </summary>
        /// <param name="lpfn">PDD call back function = nullptr</param>
        /// <param name="nDriveType">Driver type = DRIVE_REMOVABLE</param>
        /// <returns>0: Success or Error code</returns>
        static int EnumVolumeDevices(BOOL(*lpfn)(PSTORAGE_DEVICE_DESCRIPTOR) = nullptr,
                                     UINT nDriveType = DRIVE_REMOVABLE);

        static BOOL EnumDevicePath(LPCGUID pguid, PSTORAGE_DEVICE_NUMBER psdn = nullptr);
        static int EnumDiskDevicePath(PSTORAGE_DEVICE_NUMBER psdn);
    };

    //extern LIBSPARK_API int nlibsparkusb;

    //LIBSPARK_API int fnlibsparkusb(void);

    namespace sm3350
    {
        // =====================================================================
        // P1-2 root-cure: Strong-index wrappers — PhyIndex / TesterId.
        // Only static factories can construct them, and raw values outside
        // [0, MAX_DEVICE_CNT) are either clamped (FromRawChecked) or marked
        // invalid (Invalid / invalid inputs to lookup).
        //
        // Goal: compile-time prevent passing a random UCHAR (255, 20, ...)
        // into indexed APIs; runtime clamp ensures zero OOB even when callers
        // still use the legacy UCHAR overloads we keep for migration.
        // =====================================================================
        class LIBSPARK_API PhyIndex {
        public:
            // Preferred construction: validates raw; out of range => invalid.
            static PhyIndex FromRawChecked(UCHAR raw) noexcept {
                return (raw < MAX_DEVICE_CNT) ? PhyIndex(raw, true) : PhyIndex(0, false);
            }
            static PhyIndex Invalid() noexcept { return PhyIndex(0, false); }
            // Construct from an already-clamped physical index (e.g. loop counter over gu08DeviceCnt).
            static PhyIndex FromTrusted(UCHAR raw) noexcept {
                return (raw < MAX_DEVICE_CNT) ? PhyIndex(raw, true) : PhyIndex(0, false);
            }
            bool  IsValid() const noexcept { return valid_; }
            UCHAR value() const noexcept { return val_; }  // always < MAX_DEVICE_CNT when IsValid
            // Implicitly safe default: 0 when valid=false as well; never UCHAR_MAX.
        private:
            PhyIndex(UCHAR v, bool ok) noexcept : val_(v), valid_(ok) {}
            UCHAR val_;
            bool  valid_;
        };

        class LIBSPARK_API TesterId {
        public:
            static TesterId FromRawChecked(UCHAR raw) noexcept {
                return TesterId(raw < MAX_DEVICE_CNT ? raw : UCHAR(MAX_DEVICE_CNT));
            }
            static TesterId FromTrusted(UCHAR raw) noexcept {
                return (raw < MAX_DEVICE_CNT) ? TesterId(raw) : TesterId(UCHAR(MAX_DEVICE_CNT));
            }
            bool  IsValid() const noexcept { return val_ < MAX_DEVICE_CNT; }
            UCHAR value() const noexcept { return val_; }
        private:
            explicit TesterId(UCHAR v) noexcept : val_(v) {}
            UCHAR val_;
        };

        class LIBSPARK_API CSparkSm3350Util
        {
        public:
            // ---------- Primary (strongly-typed, root-cured) accessors ----------
            static CSparkSm3350Util& getInstance(PhyIndex idx) noexcept
            {
                // PhyIndex.value() is either < MAX_DEVICE_CNT (valid) or 0 (invalid).
                // So sInstance[idx.value()] is ALWAYS within [0,15] — zero OOB UB.
                static CSparkSm3350Util sInstance[MAX_DEVICE_CNT];
                static CSparkSm3350Util sFallback;  // for invalid PhyIndex: never dereferences uninit path
                return idx.IsValid() ? sInstance[idx.value()] : sFallback;
            }
            // Legacy UCHAR overload — auto-clamps, preserves backward compile compatibility.
            // No OOB possible even if caller passes 255.
            static CSparkSm3350Util& getInstance(UCHAR idxRaw) noexcept
            {
                return getInstance(PhyIndex::FromRawChecked(idxRaw));
            }

            CSparkSm3350Util();
            ~CSparkSm3350Util();

            /// <summary>
            /// Enum SM3350 device
            /// </summary>
            /// <param name="nDriverMode">0: Mass Storage mode or SMI Driver mode</param>
            /// <returns>0: Success or Error code</returns>
            static int EnumSm3350(int nDriverMode = 0);
            static PST_DEVICE_INFO GetDeviceInfo();
            // -------- Primary strong-typed entry points --------
            // P2-4 fix 1/3: ADD PhyIndex overload. This is the preferred entry point for
            // stage-level code and for legacy callers (Scan loop) that iterate by
            // physical-slot index. O(1) direct gstDeviceInfo[phys] access.
            static PST_DEVICE_INFO GetDeviceInfo(PhyIndex phys) noexcept;
            // TesterId overload: performs reverse lookup via GetPhysicalIndex (O(n) scan),
            // then delegates to the PhyIndex overload above. Use only when you truly have
            // a tester-port number (0-based Port X in the UI minus 1).
            static PST_DEVICE_INFO GetDeviceInfo(TesterId id) noexcept;
            static UCHAR          GetTesterIndex(PhyIndex phys) noexcept;  // map phys -> tester id (or UCHAR_MAX)
            static PhyIndex       GetPhysicalIndex(TesterId tester) noexcept;
            // -------- Legacy UCHAR wrappers (auto-clamped, zero OOB possible) --------
            // P2-4 fix 3/3: repoint GetDeviceInfo(UCHAR) to PHY INDEX semantics, so it
            // matches GetTesterIndex(UCHAR) (both interpret the raw UCHAR as physical
            // slot index).  This restores correct behavior for callers like the Scan
            // button loop which passes the SAME i to both GetDeviceInfo(i) and
            // GetTesterIndex(i).
            static PST_DEVICE_INFO GetDeviceInfo(UCHAR idRaw) noexcept { return GetDeviceInfo(PhyIndex::FromRawChecked(idRaw)); }
            static UCHAR          GetTesterIndex(UCHAR idRaw) noexcept { return GetTesterIndex(PhyIndex::FromRawChecked(idRaw)); }
            static UCHAR          GetPhysicalIndex(UCHAR testerRaw) noexcept {
                PhyIndex p = GetPhysicalIndex(TesterId::FromRawChecked(testerRaw));
                return p.IsValid() ? p.value() : UCHAR_MAX;
            }
            // Handy one-stop: from tester (portIndex) -> a usable PhyIndex, plus guaranteed
            // non-UOB getInstance. Prefer this at the top of Stage functions.
            static PhyIndex       ResolvePhyIndex(UCHAR testerRaw) noexcept {
                return GetPhysicalIndex(TesterId::FromRawChecked(testerRaw));
            }
            //static UCHAR GetTesterIndex(UCHAR order);

            int GetDevicePath(PhyIndex idx, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned) noexcept;
            // Legacy UCHAR overload; clamped internally; null ptr + off-by-one also eliminated.
            int GetDevicePath(unsigned char idxRaw, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned) noexcept {
                return GetDevicePath(PhyIndex::FromRawChecked(idxRaw), lpOutBuffer, nOutBufferSize, lpBytesReturned);
            }

            /// <summary>
            /// Select SM3350 device
            /// </summary>
            /// <param name="idx">index number</param>
            /// <returns>0: Success or Error code</returns>
            int DeviceSelect(PhyIndex idx) noexcept;
            int DeviceSelect(UCHAR idxRaw) noexcept { return DeviceSelect(PhyIndex::FromRawChecked(idxRaw)); }

            /************************************************************************/
            /* SMI SM3350 VCMD                                                       */
            /************************************************************************/
            int UpiuForceRom(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int VccOffForceRom(PCHAR pData = nullptr);
            int UfsPowerOn(PCHAR pData = nullptr);
            int UfsPowerOff(PCHAR pData = nullptr);
            int GetCmdResp();
            int EnterH8(PCHAR pData = nullptr);
            int ExitH8(PCHAR pData = nullptr);
            int ReadCurrent(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsMpStartMode(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsWrite1024KIspMp(PCHAR pData, UINT nSectorCnt, BOOL bEraseAllBlock);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsMpExit(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsCardInit(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsReadPortInfo(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsWritePortInfo(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsSetSrialNumberString(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsSetManuDate(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsCheckIsp(PCHAR pData = nullptr);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsCheckSram2(PCHAR pData1, PCHAR pData2);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsWriteSramMp(PCHAR pData, UINT nSectorCnt);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsReadSramResult(PCHAR pData, UINT nSectorCnt);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsReadCidInfo(PCHAR pData, UINT nSectorCnt);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsGetGeometry(PCHAR pData);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UFSReadPRV(PCHAR pData);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UFSReadID(PCHAR pData);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsReadAging(PCHAR pData);
            /// <summary>
            /// Query sm3350 information
            /// </summary>
            /// <param name="pData">Target Response buffer address</param>
            /// <returns>0: Success or Error code</returns>
            int UfsReadQ100(PCHAR pData);
        public:
            PCHAR m_szDevicePath = nullptr;

        private:
            CSm3350Vcmds m_sm3350Vcmds;
            U_MIX m_mixBuf = { 0 };
            CHAR m_pTmpBuf[512] = { 0 };
            CHAR m_pVcmdBuf[512] = { 0 };
            CHAR m_pInfo[512] = { 0 };
            PU_VCMD m_puVcmd = (PU_VCMD)m_pVcmdBuf;
        };
    }

    namespace file
    {
        /// <summary>
        /// File data read
        /// </summary>
        /// <param name="file_path">Source file path</param>
        /// <param name="dest">Destation data address</param>
        /// <param name="off">File start offset</param>
        /// <param name="size">Data length (Byte)</param>
        /// <param name="len">Return read length</param>
        /// <returns>0: Success or Error code</returns>
        LIBSPARK_API int fnReadFile(const char* file_path, char* dest,
                                    int off = 0, int size = 0, int* len = nullptr);

        /// <summary>
        /// File data write
        /// </summary>
        /// <param name="file_path">Target file path</param>
        /// <param name="src">Source data address</param>
        /// <param name="size">Data length (Byte)</param>
        /// <returns>0: Success or Error code</returns>
        LIBSPARK_API int fnWriteFile(const char* file_path, char* src, int size);

        /// <summary>
        /// Get file size (Byte)
        /// </summary>
        /// <param name="file_path">File path</param>
        /// <param name="size">File size return pointer</param>
        /// <returns>0: Success or Error code</returns>
        LIBSPARK_API int fnFileSize(const char* file_path, int* size);
    }
}