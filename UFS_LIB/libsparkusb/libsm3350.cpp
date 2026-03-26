#include "pch.h"
#include "libsparkusb.h"
#include "libsm3350.h"
#include <ntddscsi.h>
#include "SDMMC.h"
#include "Fat32Volume.h"
#include "Sm3350Volume.h"
#include "SmiDriverCmds.h"
#include "ExFatVolume.h"

#define BYTE2SEC(x)             ((x + 511) / 512)

extern uint8_t gu08DeviceCnt;
extern PST_DEVICE_INFO gstDeviceInfo;

#define SM3350_VID       "SMI"
#define SM3350_PID       "USB-UFS BRIDGE"
BOOL EnumSm3350CallbackFun(PSTORAGE_DEVICE_DESCRIPTOR psdd)
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


CSm3350Vcmds::CSm3350Vcmds()
{
    m_Buffer = (PCHAR)GlobalAlloc(GPTR, 65536 + 512);
    m_pData = (PCHAR)GlobalAlloc(GPTR, 512*16);
}

CSm3350Vcmds::~CSm3350Vcmds()
{
    CloseDeivce();

    if (m_Buffer != nullptr)
    {
        GlobalFree(m_Buffer);
    }
    if (m_pData != nullptr)
    {
        GlobalFree(m_pData);
    }
}

int CSm3350Vcmds::OpenDevice(LPSTR szDevPath, int nDriverMode /*= 0*/)
{
    if (szDevPath != nullptr)
    {
        m_nDriverMode = nDriverMode;

        if (m_pScsiCmds != nullptr)
        {
            delete m_pScsiCmds;
        }

        if (nDriverMode)
        {
            m_pScsiCmds = new CSmiDriverCmds(szDevPath);
        }
        else
        {
            m_pScsiCmds = new CScsiDriveCmds(szDevPath);
        }

        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_INVALID_ACCEL_HANDLE;
    }

    /*if (m_hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hDevice);
    }

    m_hDevice = CreateFile(szDevPath,
                           (GENERIC_WRITE | GENERIC_READ),
                           (FILE_SHARE_READ | FILE_SHARE_WRITE),
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_NO_BUFFERING,
                           NULL);

    if (m_hDevice != INVALID_HANDLE_VALUE)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return GetLastError();
    }*/
}

int CSm3350Vcmds::CloseDeivce()
{
    int ret = ERROR_SUCCESS;

    if (m_pScsiCmds != nullptr)
    {
        delete m_pScsiCmds;
    }

    return ret;
}

int CSm3350Vcmds::UfsWriteBufferUpiu(PCHAR pData, uint32_t cmd, uint32_t allocLen, UCHAR Len)
{
    if (pData == nullptr)
    {
        ZeroMemory(m_pData, sizeof(m_pData));
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_WRITE_BUFFER_UPIU);
    m_Cdb.ufs1.cmd = _byteswap_ulong(cmd);
    m_Cdb.ufs1.AllocLen = _byteswap_ulong(allocLen);
    m_Cdb.ufs1.uLen = Len;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, Len, m_Cdb);
}

int CSm3350Vcmds::UfsReadBufferUpiu(PCHAR pData, uint32_t cmd, uint32_t allocLen, UCHAR Len)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_READ_BUFFER_UPIU);
    m_Cdb.ufs1.cmd = _byteswap_ulong(cmd);
    m_Cdb.ufs1.AllocLen = _byteswap_ulong(allocLen);
    m_Cdb.ufs1.uLen = Len;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsWrite1024KIspMp(PCHAR pData, uint32_t nSectorCnt, BOOL bEraseGoodBlock)
{
    int nRet;
    int i = 0, max = nSectorCnt / 0x80;
    while (i < max)
    {
        ZeroMemory(&m_Cdb, sizeof(m_Cdb));
        m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_WRITE_1024K_ISP_MP);
        m_Cdb.ufs1.uIdx = i;
        if (0 == i)
        {
            //first time set function and length
            if (bEraseGoodBlock) {
                m_Cdb.ufs1.cmd = _byteswap_ulong(0xCBAD1160);
            }
            else {
                m_Cdb.ufs1.cmd = _byteswap_ulong(0xFBEE2260);
            }
            //allocation length
            m_Cdb.ufs1.AllocLen = _byteswap_ulong(SECTOR2BYTE(nSectorCnt));
        }
        //per 0x80 sectors(64K)
        m_Cdb.ufs1.uLen = 0x80;
        nRet = m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 0x80, m_Cdb);
        TRACE("CMD:%2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X\r\n",
            m_Cdb.ub[0x00], m_Cdb.ub[0x01], m_Cdb.ub[0x02], m_Cdb.ub[0x03], m_Cdb.ub[0x04], m_Cdb.ub[0x05], m_Cdb.ub[0x06], m_Cdb.ub[0x07],
            m_Cdb.ub[0x08], m_Cdb.ub[0x09], m_Cdb.ub[0x0A], m_Cdb.ub[0x0B], m_Cdb.ub[0x0C], m_Cdb.ub[0x0D], m_Cdb.ub[0x0E], m_Cdb.ub[0x0F]);
        TRACE("write %2.2dth 64KB file\r\n", i+1);
        pData += SECTOR2BYTE(0x80);
        ++i;
        int nTry = 0;
        if (ERROR_SUCCESS == nRet)
        {
            Sleep(100);
            nRet = GetCmdResp();
            if (ERROR_SUCCESS != nRet)
            {
                TRACE("Get Resp fail \r\n");
                return nRet;
            }

        }
        else
        {
            TRACE("write fail\r\n");
            break;
        }
    }
    return nRet;
}

int CSm3350Vcmds::UfsMpStartMode(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_UFS_MP_START_MODE);
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsCardInit(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_UFS_CARD_INIT);
    m_Cdb.ufs1.uLen = 0x01;
    m_Cdb.ub[0x0E] = 0x01;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsUpiuForceRomCodeModeForUfs(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_UPIU_FORCE_ROM_MODE_UFS);
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsVccOffForceRomModeUfs(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_VCC_OFF_FORCE_ROM_MODE_UFS);
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsPowerOn(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_UFS_POWER_ON);
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsPowerOff(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_UFS_POWER_OFF);
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}







int CSm3350Vcmds::GetCmdResp()
{
    int nRet;
    int nTry = 0;
    while (nTry < 3)
    {
        Sleep(100);
        ZeroMemory(&m_Cdb, sizeof(m_Cdb));
        m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_GET_CMD_RESP);
        m_Cdb.ufs1.uLen = 1;
        nRet = m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, m_Buffer, 1, m_Cdb);
        if (ERROR_SUCCESS == nRet)
        {
            if (0x00 != m_Buffer[0x0A] ||
                0x00 != m_Buffer[0x0B] ||
                0x00 != m_Buffer[0x07])
            {//check fail, try again
                ++nTry;
            }
            else
            {//check success
                break;
            }
        }
        else
        {//send cmd response fail
            return nRet;
        }
        if (nTry >= 3)
        {//custom error code
            nRet = 0x5E5E0000;
            nRet |= *(unsigned long*)(m_Buffer + 0x2E) << 8;
            nRet |= *(unsigned long*)(m_Buffer + 0x2F);
            return nRet;
        }
    }
    return nRet;
}

int CSm3350Vcmds::UfsWrite10(PCHAR pData, uint32_t lba, uint16_t allocLen, UCHAR Len)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs3.u16OpCode = _byteswap_ushort(CMD_WRITE_10);
    m_Cdb.ufs3.uIdx = 1;
    m_Cdb.ufs3.lba = _byteswap_ulong(lba);
    m_Cdb.ufs3.AllocLen = _byteswap_ulong(allocLen);
    m_Cdb.ufs3.uLen = Len;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsRead10(PCHAR pData, uint32_t lba, uint16_t allocLen, UCHAR Len)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs3.u16OpCode = _byteswap_ushort(CMD_READ_10);
    m_Cdb.ufs3.uIdx = 0;
    m_Cdb.ufs3.lba = _byteswap_ulong(lba);
    m_Cdb.ufs3.AllocLen = _byteswap_ulong(allocLen);
    m_Cdb.ufs3.uLen = Len;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, Len, m_Cdb);
}

int CSm3350Vcmds::UfsEnterH8(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_ENTER_H8);
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsExitH8(PCHAR pData)
{
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_EXIT_H8);
    m_Cdb.ufs1.uIdx = 1;
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsReadCurrent(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_READ_CURRENT);
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsSetGpio(PCHAR pData, UCHAR iccq_0_ohm, UCHAR iccq_1_ohm, UCHAR iccq_10_ohm, UCHAR icc_0_ohm, UCHAR icc_1_ohm, UCHAR icc_10_ohm)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_SET_GPIO);
    m_Cdb.ub[0x03] = iccq_0_ohm;
    m_Cdb.ub[0x04] = iccq_1_ohm;
    m_Cdb.ub[0x06] = iccq_10_ohm;
    m_Cdb.ub[0x07] = icc_0_ohm;
    m_Cdb.ub[0x08] = icc_1_ohm;
    m_Cdb.ub[0x0A] = icc_10_ohm;
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsReadDescriptor(PCHAR pData, UCHAR idn, UCHAR idx, UCHAR sel, UINT32 length)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.descriptor.u16OpCode = _byteswap_ushort(CMD_READ_DESCRIPTOR);
    m_Cdb.descriptor.idn = idn;
    m_Cdb.descriptor.idx = idx;
    m_Cdb.descriptor.sel = sel;
    m_Cdb.descriptor.length = _byteswap_ulong(length);
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsReadDeviceDescriptor(PCHAR pData, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x00, 0x00, 0x00, length);
}

int CSm3350Vcmds::UfsReadConfigDescriptor(PCHAR pData, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x01, 0x00, 0x00, length);
}

int CSm3350Vcmds::UfsReadRpmbUnitDescriptor(PCHAR pData, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x02, 0xC4, 0x00, length);
}

int CSm3350Vcmds::UfsReadInterconnectDescriptor(PCHAR pData, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x04, 0x00, 0x00, length);
}

int CSm3350Vcmds::UfsReadGeometryDescriptor(PCHAR pData, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x07, 0x00, 0x00, length);
}

int CSm3350Vcmds::UfsReadPowerParamDescriptor(PCHAR pData, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x08, 0x00, 0x00, length);
}

int CSm3350Vcmds::UfsReadUnitDescriptor(PCHAR pData, UCHAR idx, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x02, idx, 0x00, length);
}

int CSm3350Vcmds::UfsReadDeviceHealthDescriptor(PCHAR pData, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x09, 0x00, 0x00, length);
}

int CSm3350Vcmds::UfsReadStringDescriptor(PCHAR pData, UCHAR idx, UINT32 length)
{
    return UfsReadDescriptor(pData, 0x05, idx, 0x00, length);
}

int CSm3350Vcmds::UfsReadAttributes(PCHAR pData, UCHAR idn)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.descriptor.u16OpCode = _byteswap_ushort(CMD_READ_DESCRIPTOR);
    m_Cdb.descriptor.rsv1 = 0x02;
    m_Cdb.descriptor.idn = idn;
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsFlagRead(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.descriptor.u16OpCode = _byteswap_ushort(CMD_READ_DESCRIPTOR);
    m_Cdb.descriptor.rsv1 = 0x01;
    m_Cdb.descriptor.idn = 0xFF;
    m_Cdb.ufs1.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsTestUnitReady(PCHAR pData, UCHAR lun)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_TEST_UNIT_READY);
    m_Cdb.ub[0x03] = lun;
    m_Cdb.ub[0x0B] = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsWriteAttribute(PCHAR pData, UCHAR idn, UINT32 value)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.attribute.u16OpCode = _byteswap_ushort(CMD_WRITE_ATTRIBUTE);
    m_Cdb.attribute.idn = idn;
    m_Cdb.attribute.value = _byteswap_ulong(value);
    m_Cdb.attribute.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsWriteConfigDescriptorOrSetPartition(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_WRITE_CONFIG_DESCRIPTOR_OR_SET_PARTITION);
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsConfigReferenceClock(PCHAR pData, UCHAR idn)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ref_clock.u16OpCode = _byteswap_ushort(CMD_CONFIG_REFERENCE_CLOCK);
    m_Cdb.ref_clock.value = _byteswap_ulong(0x00020122);
    m_Cdb.ref_clock.idn = idn;
    m_Cdb.ref_clock.uLen = 1;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 1, m_Cdb);
}

int CSm3350Vcmds::UfsMpExit(PCHAR pData)
{
    //return UfsWriteBufferUpiu(pData, 0x45584954, 0x00001000, 0x08);
    return UfsWriteBufferUpiu(pData, 0x45584954, 0x00000000, 0x00);
}

int CSm3350Vcmds::UfsReadPortInfo(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_Buffer;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_READ_PORT_ID);
    m_Cdb.ub[0x03] = 0x80;
    m_Cdb.ub[0x09] = 0x00;
    m_Cdb.ub[0x0B] = 0x02;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 2, m_Cdb);
}

int CSm3350Vcmds::UfsWritePortInfo(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_Buffer;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_WRITE_PORT_ID);
    m_Cdb.ub[0x03] = 0x80;
    m_Cdb.ub[0x09] = 0x01;
    m_Cdb.ub[0x0B] = 0x02;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 2, m_Cdb);
}

int CSm3350Vcmds::UfsVcmdStart(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(pData, 512*8);
    pData[0] = 0x4e; pData[1] = 0x50; pData[2] = 0x4f; pData[3] = 0x56;
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_READ_BUFFER_UPIU);
    m_Cdb.ufs1.cmd = _byteswap_ulong(0x004D4554);
    m_Cdb.ufs1.AllocLen = _byteswap_ulong(0x00001000);
    m_Cdb.ufs1.uLen = 0x08;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 0x08, m_Cdb);
}

int CSm3350Vcmds::UfsVcmdEnd(PCHAR pData)
{
    if (pData == nullptr)
    {
        pData = m_pData;
    }
    ZeroMemory(pData, 512 * 8);
    pData[0] = 0x4f; pData[1] = 0x4c; pData[2] = 0x43; pData[3] = 0x56;
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_READ_BUFFER_UPIU);
    m_Cdb.ufs1.cmd = _byteswap_ulong(0x004D4552);
    m_Cdb.ufs1.AllocLen = _byteswap_ulong(0x00001000);
    m_Cdb.ufs1.uLen = 0x08;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 0x08, m_Cdb);
}

int CSm3350Vcmds::UfsVcmdWrite(PCHAR pData, UCHAR flag)
{
    if (pData == nullptr)
    {
        pData = m_Buffer;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.vcmd.u16OpCode = _byteswap_ushort(CMD_VCMD_WRITE);
    m_Cdb.vcmd.flag = flag;
    m_Cdb.vcmd.uLen = 0x08;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 0x08, m_Cdb);
}

int CSm3350Vcmds::UfsVcmdRead(PCHAR pData, UCHAR flag)
{
    if (pData == nullptr)
    {
        pData = m_Buffer;
    }
    ZeroMemory(&m_Cdb, sizeof(m_Cdb));
    m_Cdb.vcmd.u16OpCode = _byteswap_ushort(CMD_VCMD_READ);
    m_Cdb.vcmd.flag = flag;
    m_Cdb.vcmd.uLen = 0x08;
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, pData, 0x08, m_Cdb);
}

int CSm3350Vcmds::UfsWriteSramMp(PCHAR pData, UINT nSectorCnt)
{
    int nRet;
    do
    {
        ZeroMemory(&m_Cdb, sizeof(m_Cdb));
        m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_WRITE_SRAM_MP);
        m_Cdb.ufs1.uIdx = 0x00;
        m_Cdb.ufs1.cmd = _byteswap_ulong(0xFBEE2260);
        //allocation length
        m_Cdb.ufs1.AllocLen = _byteswap_ulong(0x00100000);
        m_Cdb.ufs1.uLen = 0x80;
        if ((nRet = m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 0x80, m_Cdb)) != ERROR_SUCCESS) break;
        Sleep(100);
        if ((nRet = GetCmdResp()) != ERROR_SUCCESS) break;
        ZeroMemory(&m_Cdb, sizeof(m_Cdb));
        m_Cdb.ufs1.u16OpCode = _byteswap_ushort(CMD_WRITE_SRAM_MP);
        m_Cdb.ufs1.uIdx = 0x11;
        m_Cdb.ufs1.uLen = 0x80;
        pData+= SECTOR2BYTE(0x80)*0x11;
        if ((nRet = m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, pData, 0x80, m_Cdb)) != ERROR_SUCCESS) break;

    } while (0);
    
    return nRet;
}


int CSm3350Vcmds::ScsiCmdDataIn(PCHAR dataBuffer)
{
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_IN, dataBuffer, 1, m_Cdb);
}

int CSm3350Vcmds::ScsiCmdDataOut(PCHAR dataBuffer)
{
    return m_pScsiCmds->ScsiSendCmd(SCSI_IOCTL_DATA_OUT, dataBuffer, 1, m_Cdb);
}
