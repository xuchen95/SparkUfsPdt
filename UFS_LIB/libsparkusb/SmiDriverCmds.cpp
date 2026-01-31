#include "pch.h"
#include "SmiDriverCmds.h"

CSmiDriverCmds::CSmiDriverCmds(CHAR* devPath)
{
    m_pszBuf = devPath;
    for (size_t i = 0; i < ARRAYSIZE(m_hPipe); i++)
    {
        m_hPipe[i] = INVALID_HANDLE_VALUE;
    }
}

CSmiDriverCmds::~CSmiDriverCmds()
{
    CloseDevice();
}

int CSmiDriverCmds::ScsiSendCmd(UCHAR _DataIn, PCHAR dataBuffer, UCHAR _SectorCnt, U_CDB& cdb)
{
    return ScsiSendCmdByte(_DataIn, dataBuffer, (_SectorCnt << 9), cdb);
}

int CSmiDriverCmds::ScsiSendCmdByte(UCHAR _DataIn, PCHAR dataBuffer, UINT _ByteCnt, U_CDB& cdb)
{
    CBW cbw = { 0 };
    CSW csw;
    DWORD dwBytes;
    int iRet = NO_ERROR;

    if (NO_ERROR == OpenDevice())
    {
        do
        {
            // prepare CBW package data
            cbw.dwCBWSignature = USB_CBW_SIGNATURE;
            cbw.dwCBWXferLength = _ByteCnt;
            if (_DataIn)
            {
                cbw.bCBWFlags = 0x80;
            }
            cbw.bCBWCBLength = 10;
            memcpy(cbw.cdb.AsByte, cdb.ub, sizeof(cdb));

            // send CBW and get CSW
            if (!WriteFile(m_hPipe[1], &cbw, sizeof(cbw), &dwBytes, NULL))
            {
                iRet = GetLastError();
                break;
            }

            // read/write target data
            if (_DataIn)
            {
                if (!ReadFile(m_hPipe[0], dataBuffer, _ByteCnt, &dwBytes, NULL))
                {
                    iRet = GetLastError();
                    break;
                }
            }
            else
            {
                if (!WriteFile(m_hPipe[1], dataBuffer, _ByteCnt, &dwBytes, NULL))
                {
                    iRet = GetLastError();
                    break;
                }
            }

            // get CSW package data
            if (!ReadFile(m_hPipe[0], &csw, sizeof(csw), &dwBytes, NULL))
            {
                iRet = GetLastError();
                break;
            }

            if (csw.bCSWStatus)
            {
                iRet = ERROR_FUNCTION_FAILED;
            }
        } while (0);

        CloseDevice();
    }

    return iRet;
}

int CSmiDriverCmds::OpenDevice()
{
    CHAR szPath[MAX_PATH];
    int iRet = NO_ERROR;

    memset(szPath, 0x00, sizeof(szPath));
    for (size_t i = 0; i < ARRAYSIZE(m_hPipe); i++)
    {
        if (m_hPipe[i] != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hPipe[i]);
        }

        strcpy_s(szPath, m_pszBuf);
        strcat_s(szPath, "\\");
        strcat_s(szPath, "PIPE0");
        szPath[strlen(szPath) + 1] = 0x00;
        szPath[strlen(szPath)] = '0' + (CHAR)i;

        //strcpy(szPath, m_pszBuf);
        //strcat(szPath, "\\");
        //strcat(szPath, "PIPE0");
        //szPath[strlen(szPath)] = '0' + i;

        m_hPipe[i] = CreateFile(szPath,
                                (GENERIC_WRITE | GENERIC_READ),
                                (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_NO_BUFFERING,
                                NULL);

        if (m_hPipe[i] == INVALID_HANDLE_VALUE)
        {
            iRet = GetLastError();
        }
    }

    return iRet;
}

void CSmiDriverCmds::CloseDevice()
{
    for (size_t i = 0; i < ARRAYSIZE(m_hPipe); i++)
    {
        if (m_hPipe[i] != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hPipe[i]);
            m_hPipe[i] = INVALID_HANDLE_VALUE;
        }
    }
}

