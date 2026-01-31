#pragma once
#include <intsafe.h>

#define fnSendCmd60                 SendCmd60
#define fnRead                      SendReadCmd
#define fnWrite                     SendWriteCmd

#define SM2706_VCMD_LBA             (0x55aa)
#define M_VcmdRead(c, d)            fnRead(SM2706_VCMD_LBA, (c), (d))
#define M_VcmdWrite(c, d)           fnWrite(SM2706_VCMD_LBA, (c), (d))

#define M_SetVcmdOp(c)   \
    do {    \
        memset(m_puVcmd, 0x00, 512);   \
        m_puVcmd->st.u16OpCode = (c);    \
    } while (0);

#define M_VcmdStart()   \
    do {    \
        fnSendCmd60(m_pTmpBuf); \
        M_VcmdWrite(1, m_pVcmdBuf);   \
    } while (0);

#define DCCM_BASE_ADDR      0xC0000000
#define REG_BASE_ADDR       0xE0000000
#define INFO_RAM_ADDR       (DCCM_BASE_ADDR + 0x1000)

#define VCMD_SRAM_OP            0x0004
#define SRAM_ICCM       0x01    // 0x0001_0000 - 0x0001BFFF
#define SRAM_DCCM       0x02    // 0xC000_0000 - 0xC000_3FFF
#define SRAM_BCT        0x03    // 0xE006_0000 - 0xE006_0FFF, 4KB
#define SRAM_PRAMA      0x05    // 0xF000_0000 - 0xF000_1FFF, 8KB
#define SRAM_PRAMB      0x07    // 0xF000_2000 - 0xF000_9FFF, 32KB
#define SRAM_PRAMC      0x08    // 0xF000_A000 - 0xF001_1FFF, 32KB
#define SRAM_TSB0       0x09    // 0xF002_0000 - 0xF002_FFFF, 128KB

#define VCMD_FLASH_ID_OP        0x0006
#define VCMD_CALLADDR_OP        0x000A
#define VCMD_MTEST_OP           0x000B
#define MTEST_ICCM      0x01
#define MTEST_DCCM      0x02
#define MTEST_BCT       0x03
#define MTEST_CPRM      0x04
#define MTEST_PRAMA     0x05
#define MTEST_TSB       0x06
#define MTEST_PRAMB     0x07
#define MTEST_PRAMC     0x08

#define VCMD_HW_REG_OP          0x0040
#define REG_SYSTEM      0x01

#define VCMD_WR_BLOCK_OP        0x0101
#define VCMD_ER_BLOCK_OP        0x0102
#define VCMD_RTBB_OP            0x010A
#define VCMD_EC_CNT_OP          0x010C
#define VCMD_READ_BLOCK_OP      0x0301

#define SCMD_OP_MASK            0x8000

#define SCMD_UPDATE_SETTING     0x8001
#define SCMD_ERASE_ALL          0x800F

#define SCMD_PREFORMAT_GO       0x8010
#define SCMD_SYSTEM_WR          0x8011
#define SCMD_ISP_WR             0x8012
#define SCMD_ISP_ER             0x8013
#define SCMD_LOAD_AGING         0x8014

#define SCMD_READ_UNIQUE_ID     0x8100

#define SCMD_CHECK_BUSY         0x8200

#pragma pack(1)
typedef union
{
    struct
    {
        unsigned short u16OpCode;

        union
        {
            struct
            {
                unsigned char u08Type;
                unsigned char u08Rsv;
                unsigned short u16Addr;
                unsigned char u08Cnt;
            } SramRw;

            struct
            {
                unsigned short u16Len;
                unsigned int u32Addr;
            } RegRw;

            struct
            {
                unsigned short u16FBlock;
                unsigned short u16FPage;
                unsigned char u08Ce : 4;
                unsigned char u08FPlane : 4;

                unsigned char fIntlv : 1;
                unsigned char fTraPage : 1;
                unsigned char fTraTab : 1;
                unsigned char fStrongPage : 1;
                unsigned char fDisShapSeed : 1;

                unsigned char u08DataLen;
                unsigned char fReadData : 1;    // 0
                unsigned char fReadMeta : 1;    // 1
                unsigned char fMutiRead : 1;    // 2
                unsigned char fReadSys1 : 1;    // 3
                unsigned char fLoadToDCCM : 1;  // 4
                unsigned char : 1;              // 5
                unsigned char fReadBoot : 1;    // 6
                unsigned char : 1;
            } FBlock;

            struct
            {
                unsigned short u16Rsv;
                unsigned short u16Addr; // offset of ICCM (start from 0x00010000)
            } CallAddr;

            struct
            {
                unsigned short u16Len;
            } WriteSys;

            struct
            {
                unsigned short u16Len;
                unsigned char u08IspNo;
            } ReadIsp;

            struct
            {
                unsigned char u08MPlnMode;
                unsigned char u08TlcMode;
            } EraseAll;

            struct
            {
                unsigned char u08MPlnMode;
                unsigned char u08TlcMode;
                unsigned char u08ScanDefect;
                unsigned char u08EraseFailRetry;
                unsigned char u08CardGrade;
            } PrefGo;
        } u;
    } st;

    char ub[32];
} U_VCMD, * PU_VCMD;
#pragma pack()