#include "pch.h"
#include "ScsiDriveCmds.h"
#include "spti.h"
#include <ntddscsi.h>

CScsiDriveCmds::CScsiDriveCmds(PCHAR szPath)
{
    if (szPath != nullptr)
    {
        m_pszBuf = szPath;

        /*m_hDevice = CreateFile(m_pszBuf,
            (GENERIC_READ),
            (FILE_SHARE_READ | FILE_SHARE_WRITE),
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_NO_BUFFERING,
            NULL);*/

        m_hDevice = CreateFile(m_pszBuf,
                               (GENERIC_WRITE | GENERIC_READ),
                               (FILE_SHARE_READ | FILE_SHARE_WRITE),
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_NO_BUFFERING,
                               NULL);
        if (m_hDevice == INVALID_HANDLE_VALUE)
        {
            TRACE("Open device %s fail, error code: %lu\n", m_pszBuf, GetLastError());
        }
    }
}

CScsiDriveCmds::~CScsiDriveCmds()
{
    if (m_hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hDevice);
    }
}

int CScsiDriveCmds::ScsiSendCmd(UCHAR dataIn, PCHAR dataBuffer, UCHAR sectorCnt, U_CDB& cdb)
{
    int nRet;
    int nRetry = 3;

    do
    {
        if (ERROR_SUCCESS ==
            (nRet = ScsiSendCmdByte(dataIn, dataBuffer, ((UINT)sectorCnt) << 9, cdb)))
        {
            break;
        }
        Sleep(50);
    } while (nRetry--);

    return nRet;
}

int CScsiDriveCmds::ScsiSendCmdByte(UCHAR dataIn, PCHAR dataBuffer, UINT byteCnt, U_CDB& cdb)
{
    BOOL status = FALSE;
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    ULONG errorCode = 0, byReturned = 0;

    if (m_hDevice == INVALID_HANDLE_VALUE)
    {
        return ERROR_HANDLE_NO_LONGER_VALID;
    }

    // send SCSI command
    ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB16GENERIC_LENGTH;
    sptdwb.sptd.DataIn = dataIn;
    sptdwb.sptd.SenseInfoLength = SPT_SENSE_LENGTH;
    sptdwb.sptd.DataTransferLength = byteCnt;
    sptdwb.sptd.TimeOutValue = 30;
    sptdwb.sptd.DataBuffer = dataBuffer;
    sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
    CopyMemory(sptdwb.sptd.Cdb, cdb.ub, sizeof(sptdwb.sptd.Cdb));
    status = DeviceIoControl(m_hDevice,
                             IOCTL_SCSI_PASS_THROUGH_DIRECT,
                             &sptdwb,
                             sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
                             &sptdwb,
                             sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
                             &byReturned,
                             FALSE);

    if (sptdwb.sptd.ScsiStatus || (status == 0))
    {
        errorCode = GetLastError();
    }

    return errorCode;
}
