#pragma once
#include "ScsiDriveCmds.h"
#include <ntddscsi.h>

#define SM3350_MAX_DEVICE_CNT    16

#define CMD_WRITE_BUFFER_UPIU 0xD2E0
#define CMD_READ_BUFFER_UPIU 0xD2E1
#define CMD_WRITE_1024K_ISP_MP 0xD0F0
#define CMD_UFS_MP_START_MODE 0xD1E1
#define CMD_UFS_CARD_INIT 0xD1E2
#define CMD_UPIU_FORCE_ROM_MODE_UFS 0xD1E0
#define CMD_VCC_OFF_FORCE_ROM_MODE_UFS 0xD1E4
#define CMD_UFS_POWER_ON 0xD404
#define CMD_UFS_POWER_OFF 0xD406
#define CMD_GET_CMD_RESP 0xD500
#define CMD_WRITE_10 0xD1E6
#define CMD_READ_10 0xD1E7
#define CMD_ENTER_H8 0xD405
#define CMD_EXIT_H8 0xD405
#define CMD_READ_CURRENT 0xD0FC
#define CMD_SET_GPIO 0xD0F8
#define CMD_READ_DESCRIPTOR 0xD1EC
#define CMD_READ_DEVICE_DESCRIPTOR 0xD1EC
#define CMD_READ_CONFIG_DESCRIPTOR 0xD1EC
#define CMD_READ_RPMB_UNIT_DESCRIPTOR 0xD1EC
#define CMD_READ_INTERCONNECT_DESCRIPTOR 0xD1EC
#define CMD_READ_GEOMETRY_DESCRIPTOR 0xD1EC
#define CMD_READ_POWER_PARAM_DESCRIPTOR 0xD1EC
#define CMD_READ_UNIT_DESCRIPTOR 0xD1EC
#define CMD_READ_DEVICE_HEALTH_DESCRIPTOR 0xD1EC
#define CMD_READ_STRING_DESCRIPTOR 0xD1EC
#define CMD_READ_ATTRIBUTES 0xD1EC
#define CMD_READ_FLAGS 0xD1EC
#define CMD_TEST_UNIT_READY 0xD1F1
#define CMD_WRITE_ATTRIBUTE 0xD1EB
#define CMD_WRITE_CONFIG_DESCRIPTOR_OR_SET_PARTITION 0xD1EA
#define CMD_CONFIG_REFERENCE_CLOCK 0xD305
#define CMD_WRITE_PORT_ID 0xF0E0
#define CMD_READ_PORT_ID 0xF0E1

#define CMD_VCMD_WRITE 0xD1E6
#define CMD_VCMD_READ 0xD1E7

#define SECTOR2BYTE(x)              ((x) << 9)
#define BYTE2SECTOR(x)              ((x + 511) >> 9)

#define FLAG_WRITE_MDT (0xEC)
#define FLAG_WRITE_PSN (0xEE)
#define FLAG_CHECK_ISP (0xA4)
#define FLAG_CHECK_SRAM1 (0xA6)
#define FLAG_CHECK_SRAM2 (0xA7)

class CSm3350Vcmds
{
public:
    CSm3350Vcmds();
    ~CSm3350Vcmds();
    int OpenDevice(LPSTR szDevPath, int nDriverMode = 0);
    int CloseDeivce();

    int UfsWriteBufferUpiu(PCHAR pData, uint32_t cmd, uint32_t allocLen, UCHAR Len);
    int UfsReadBufferUpiu(PCHAR pData, uint32_t cmd, uint32_t allocLen, UCHAR Len);
    int UfsWrite1024KIspMp(PCHAR pData, uint32_t nSectorCnt, BOOL bEraseAllBlock);
    int UfsMpStartMode(PCHAR pData);
    int UfsCardInit(PCHAR pData);
    int UfsUpiuForceRomCodeModeForUfs(PCHAR pData);
    int UfsVccOffForceRomModeUfs(PCHAR pData);
    int UfsPowerOn(PCHAR pData);
    int UfsPowerOff(PCHAR pData);
    int GetCmdResp();
    int UfsWrite10(PCHAR pData, uint32_t lba, uint16_t allocLen,UCHAR Len);
    int UfsRead10(PCHAR pData, uint32_t lba, uint16_t allocLen, UCHAR Len);
    int UfsEnterH8(PCHAR pData);
    int UfsExitH8(PCHAR pData);
    int UfsReadCurrent(PCHAR pData);
    int UfsSetGpio(PCHAR pData,UCHAR iccq_0_ohm, UCHAR iccq_1_ohm, UCHAR iccq_10_ohm, UCHAR icc_0_ohm, UCHAR icc_1_ohm, UCHAR icc_10_ohm);
    int UfsReadDescriptor(PCHAR pData, UCHAR idn, UCHAR idx, UCHAR sel, UINT32 length);
    int UfsReadDeviceDescriptor(PCHAR pData, UINT32 length);
    int UfsReadConfigDescriptor(PCHAR pData, UINT32 length);
    int UfsReadRpmbUnitDescriptor(PCHAR pData, UINT32 length);
    int UfsReadInterconnectDescriptor(PCHAR pData, UINT32 length);
    int UfsReadGeometryDescriptor(PCHAR pData, UINT32 length);
    int UfsReadPowerParamDescriptor(PCHAR pData, UINT32 length);
    int UfsReadUnitDescriptor(PCHAR pData, UCHAR idx, UINT32 length);
    int UfsReadDeviceHealthDescriptor(PCHAR pData, UINT32 length);
    int UfsReadStringDescriptor(PCHAR pData, UCHAR idx, UINT32 length);
    int UfsReadAttributes(PCHAR pData, UCHAR idn);
    int UfsFlagRead(PCHAR pData);
    int UfsTestUnitReady(PCHAR pData,UCHAR lun);
    int UfsWriteAttribute(PCHAR pData, UCHAR idn, UINT32 value);
    int UfsWriteConfigDescriptorOrSetPartition(PCHAR pData);
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="idn"> '0x00  = 19.2 MHz   0x01 = 26MHz '</param>
    /// <returns></returns>
    int UfsConfigReferenceClock(PCHAR pData, UCHAR idn);
    int UfsMpExit(PCHAR pData);

    int UfsReadPortInfo(PCHAR pData);
    int UfsWritePortInfo(PCHAR pData);

    int UfsVcmdStart(PCHAR pData);
    int UfsVcmdEnd(PCHAR pData);
    int UfsVcmdWrite(PCHAR pData, UCHAR flag);
    int UfsVcmdRead(PCHAR pData, UCHAR flag);

protected:
    int ScsiCmdDataIn(PCHAR dataBuffer);
    int ScsiCmdDataOut(PCHAR dataBuffer);

    CScsiDriveCmds* m_pScsiCmds = nullptr;

    U_CDB m_Cdb = { 0 };
    PCHAR m_Buffer = nullptr;
    int m_nDriverMode;
};