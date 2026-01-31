#pragma once
#include <winioctl.h>

using namespace std;

#pragma pack(1)
typedef union
{

    struct
    {
        USHORT u16OpCode;       //Byte 0-1
        UCHAR uIdx;              //Byte 2
        UINT32 cmd;          //Byte 3-6
        UINT32 AllocLen;          //Byte 7-A
        UCHAR uLen;             // Byte B
    }ufs1;
    struct
    {
        USHORT u16OpCode;       //Byte 0-1
        UCHAR uIdx;              //Byte 2
        UINT32 cmd;          //Byte 3-6
        UINT32 AllocLen;          //Byte 7-A
        UCHAR uUfs_rsv4;             //Byte B
        UCHAR uLen;                 //ByteC
    }ufs2;
    struct
    {
        USHORT u16OpCode;       //Byte 0-1
        UCHAR uIdx;              //Byte 2
        UINT32 lba;          //Byte 3-6
        UINT32 AllocLen;          //Byte 7-A
        UCHAR uLen;             //Byte B
    }ufs3;
    struct
    {
        USHORT u16OpCode;       //Byte 0-1
        UCHAR rsv1;              //Byte 2
        UCHAR idn;              //Byte 3
        UCHAR idx;          //Byte 4
        UCHAR sel;          //Byte 5
        UINT32 length;          //Byte 6-9
        UCHAR rsv2;              //Byte A
        UCHAR uLen;             // Byte B
    }descriptor;
    struct
    {
        USHORT u16OpCode;       //Byte 0-1
        UCHAR idn;              //Byte 2
        UINT32 value;              //Byte 3-6
        UINT32 rsv1;          //Byte 7-A
        UCHAR uLen;          //Byte B
    }attribute;
    struct
    {
        USHORT u16OpCode;       //Byte 0-1
        UCHAR rsv1;              //Byte 2
        UINT32 value;              //Byte 3-6
        UCHAR idn;          //Byte 7
        UCHAR rsv2;         //Byte 8    
        UCHAR rsv3;         //Byte 9
        UCHAR rsv4;         //Byte A
        UCHAR uLen;          //Byte B
    }ref_clock;
    UCHAR ub[16];
} U_CDB, *PU_CDB;
#pragma pack()

class CScsiDriveCmds
{
public:
    CScsiDriveCmds() = default;
    CScsiDriveCmds(PCHAR szPath);
    ~CScsiDriveCmds();

    virtual int ScsiSendCmd(UCHAR dataIn, PCHAR dataBuffer, UCHAR sectorCnt, U_CDB& cdb);
    virtual int ScsiSendCmdByte(UCHAR dataIn, PCHAR dataBuffer, UINT byteCnt, U_CDB& cdb);
    HANDLE GetHandle() { return m_hDevice; }

protected:
    PCHAR m_pszBuf = nullptr;

private:
    HANDLE m_hDevice = INVALID_HANDLE_VALUE;
};