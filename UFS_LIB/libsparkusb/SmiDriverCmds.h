#pragma once
#include "usbstor.h"
#include "scsi.h"
#include "ScsiDriveCmds.h"
#include <initguid.h>

DEFINE_GUID(GUID_CLASS_I82930_BULK,
    0x62152103, 0x2103, 0x11d8, 0xa5, 0x24, 0x0, 0xc, 0x76, 0x12, 0x18, 0x47);

#define GUID_DEVINTERFACE_SMI_SM3350             GUID_CLASS_I82930_BULK
#define SM3350_INQUIRY_LEN                       (0x24)

class CSmiDriverCmds : public CScsiDriveCmds
{
public:
    CSmiDriverCmds(CHAR* szPath);
    ~CSmiDriverCmds();

    int ScsiSendCmd(UCHAR _DataIn, PCHAR dataBuffer, UCHAR _SectorCnt, U_CDB& cdb) override;
    int ScsiSendCmdByte(UCHAR _DataIn, PCHAR dataBuffer, UINT _ByteCnt, U_CDB& cdb) override;

private:
    int OpenDevice();
    void CloseDevice();

    HANDLE m_hPipe[2];
};

