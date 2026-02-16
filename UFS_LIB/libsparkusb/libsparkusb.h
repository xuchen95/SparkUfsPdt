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
//#define MAX_TESTER_LUN          (2)
//#define MAX_TESTER_PER_LUN      (8)

#define MAX_DEVICE_CNT          (16)
#define MAX_TINY_CODE_SIZE      (32768)
#define MAX_BOOT_INFO_SIZE      (3072)

#define UFS_ISP_SIZE     (1024 *512 *2)


#define UFS_ERASE_ALL_BLOCK    (0)
#define UFS_ERASE_GOOD_BLOCK    (1)

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
        class LIBSPARK_API CSparkSm3350Util
        {
        public:
            static CSparkSm3350Util& getInstance(UCHAR idx)
            {
                static CSparkSm3350Util sInstance[MAX_DEVICE_CNT];
                return sInstance[idx];
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
            static PST_DEVICE_INFO GetDeviceInfo(UCHAR id);
            static UCHAR GetTesterIndex(UCHAR id);
            static UCHAR GetPhysicalIndex(UCHAR testerIdx);
            //static UCHAR GetTesterIndex(UCHAR order);

            int GetDevicePath(unsigned char idx, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);

            /// <summary>
            /// Select SM3350 device
            /// </summary>
            /// <param name="idx">index number</param>
            /// <returns>0: Success or Error code</returns>
            int DeviceSelect(UCHAR idx);

            /************************************************************************/
            /* SMI SM3350 VCMD                                                       */
            /************************************************************************/
            int UfsUpiuForceRomCodeModeForUfs(PCHAR pData = nullptr);
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
            int UfsCheckSram(PCHAR pData = nullptr);

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