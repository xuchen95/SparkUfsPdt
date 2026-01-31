#ifndef SDSTRUCT_H
#define SDSTRUCT_H

#ifdef WIN32
#define _Uncached
#endif /* WIN32 */

// Type definition
typedef unsigned char               u8;         //size:  8 bits
typedef unsigned short              u16;        //size: 16 bits
typedef unsigned int                u32;        //size: 32 bits
typedef unsigned long long          u64;        //size: 64 bits

typedef signed char                 s8;         //size:  8 bits
typedef signed short                s16;        //size: 16 bits
typedef signed int                  s32;        //size: 32 bits
typedef signed long long            s64;        //size: 64 bits

typedef u8                          uint8;
typedef u16                         uint16;
typedef u32                         uint32;
typedef u64                         uint64;

typedef s8                          int8;
typedef s16                         int16;
typedef s32                         int32;
typedef s64                         int64;

typedef u8                          BYTE;
typedef u16                         WORD;
typedef u32                         LWORD;
typedef u64                         LLWORD;

typedef s8                          SBYTE;
typedef s16                         SWORD;
typedef s32                         SLWORD;

typedef _Uncached u8                UCBYTE;
typedef _Uncached u16               UCWORD;
typedef _Uncached u32               UCLWORD;

typedef _Uncached s8                UCSBYTE;
typedef _Uncached s16               UCSWORD;
typedef _Uncached s32               UCSLWORD;

typedef _Uncached volatile u8       UCVBYTE;
typedef _Uncached volatile u16      UCVWORD;
typedef _Uncached volatile u32      UCVLWORD;

typedef _Uncached volatile s8       UCVSBYTE;
typedef _Uncached volatile s16      UCVSWORD;
typedef _Uncached volatile s32      UCVSLWORD;

typedef const u8                    CBYTE;
typedef const u16                   CWORD;
typedef const u32                   CLWORD;

#ifndef WIN32
#define NULL                        ((void *)0)
#endif // WIN32
#define CALL_FUNCTION(x)            ((void(*)(void))x)()
#define XBYTE_Unit                  ((unsigned char volatile *)0)

//Mutually exclusive
#define SM2705AA                    (0)
#define SM2705AB                    (0)
#define SM2705AC                    (0)
#define SM2705BA                    (0)
#define SM2706AA                    (1)

#define ECC_LDPC                    (SM2706AA)

#define TSB_128KB                   (SM2706AA)

//#include "Reg_Sys.h"                //System Control Register       (Start Address :0xE000_0000; Max Offset: 0x7F)
//#include "Reg_Pad.h"                //PAD Control Register          (Start Address :0xE001_0000; Max Offset: 0x7F)
//#include "Reg_TSB.h"                //TSB Control Register          (Start Address :0xE002_0000; Max Offset: 0x0F, sb_reg_bank=0)
//#include "Reg_SD.h"                 //SD interface control register (Start Address :0xE003_0000)
//#include "Reg_NFC_Global.h"         //NFC Global Control Register   (Start Address :0xE004_0000; Max Offset: 0x5F)
//#include "Reg_NFCCMDQ.h"            //NFC CMDQ Register             (Start Address :0xE004_0100; Max Offset: 0x100)
//#include "Reg_NFC1.h"               //NFC 1 Control Register        (Start Address :0xE004_0200; Max Offset: 0xFF)
//#include "Reg_DMA.h"                //Other DMA Control Register    (Start Address :0xE005_0000; Max Offset: 0x7F)
//#include "Reg_CPRM.h"               //CPRM control register         (Start Address :0xE008_0000)
//#include "Reg_SD_UHSII.h"           //UHSII Control Register        (Start Address :0xE009_0000; Max Offset: 0xC_6C)
//
//#include "ROMGlobalVar.h"
//#include "ROMInfoStruct.h"
//#include "RomFunPoint.h"

//the base unit of GetPLL() and GetCPUFreq() is MHz
#define GetPLL()                    ((((WORD)((RB_SYSTEM[rcB_AIP_PLL_S]&0x3F)+2))*(1000>>(RB_SYSTEM[rcB_AIP_PLL_S]>>6)))/100)
#define GetCPUFreq()                (GetPLL())
#define BAUDRATE                    (921600)

#define ocB_UARTParity_NoParity     0
#define ocB_UARTParity_OddParity    1
#define ocB_UARTParity_EvenParity   2

#define ocB_UARTStopBit_1bit        1
#define ocB_UARTStopBit_2bit        2

#define cTSBRam                     1

//RWOption
#define cReadData                   0x01    //cB_BIT0
#define cReadMeta                   0x02    //cB_BIT1
#define cReadDataNeedMultiRead      0x04    //cB_BIT2
#define cReadSys1FBlock             0x08    //cB_BIT3
#define cReadNeedLoadToDCCM         0x10    //cB_BIT4   // use 2KB DCCM (Program RAM A: 0xF000_0000 ~ 0xF000_07FF) as temp buffer, ex: isp swap, flash->tsb->dccm->iccm
#define cReadwithReliableMode       0x20    //cB_BIT5
#define cReadBootBlock              0x40    //cB_BIT6
#define cReadNeedtoChkColumnAddr    0x80    //cB_BIT7

#define cProgMetaSrcHostPage        0x02    //cB_BIT1   //cAltBasicIOFunc
#define cProgDataNeedMultiWrite     0x04    //cB_BIT2
#define cProgReadSys1FBlock         0x08    //cB_BIT3
#define cProgUserData               0x10    //cB_BIT4   //0: Table data; 1: User data  //cAltBasicIOFunc
#define cProgwithReliableMode       0x20    //cB_BIT5
#define cProgBootBlock              0x40    //cB_BIT6
#define cProgDataCheckBusy          0x80    //cB_BIT7

#endif //SDSTRUCT_H