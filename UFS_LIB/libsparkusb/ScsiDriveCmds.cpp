#include "pch.h"
#include "ScsiDriveCmds.h"
#include "spti.h"
#include <ntddscsi.h>

namespace
{
    void TraceHexLine(const char* title, const BYTE* data, size_t len)
    {
        char line[256] = { 0 };
        char tmp[8] = { 0 };
        size_t pos = 0;

        pos += sprintf_s(line + pos, sizeof(line) - pos, "%s", title);
        for (size_t i = 0; i < len && pos < sizeof(line) - 4; ++i)
        {
            sprintf_s(tmp, "%02X ", data[i]);
            pos += sprintf_s(line + pos, sizeof(line) - pos, "%s", tmp);
        }
        TRACE("%s\n", line);
    }
}

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
        Sleep(100);
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
    sptdwb.sptd.TimeOutValue = 200;
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
        TRACE("ScsiSendCmdByte failed: dataIn=%u byteCnt=%u status=%d scsiStatus=0x%02X lastError=0x%08lX bytesReturned=%lu\n",
            dataIn, byteCnt, status, sptdwb.sptd.ScsiStatus, errorCode, byReturned);
        TraceHexLine("  CDB: ", sptdwb.sptd.Cdb, sizeof(sptdwb.sptd.Cdb));
        TraceHexLine("  Sense: ", sptdwb.ucSenseBuf, SPT_SENSE_LENGTH);
    }

    return errorCode;
}
