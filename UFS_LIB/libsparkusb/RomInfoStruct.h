#ifndef ROMINFOSTRUCT_H
#define ROMINFOSTRUCT_H

#include "SDstruct.h"
//Boot Info Structure
#define cBootInfoSize       3072
#define cSeedTableSize      1024
#define cStrongPageSize     256

//seed table    0xC000_1000
//info table    0xC000_1800
//Strong table  0xC000_1A00

typedef struct _tBootInfo
{
    //Random Seed Table 0xC000_1000-0xC000_17FF
    WORD    XW_InfoRSTable[cSeedTableSize];     //0xC000_1000 //2*1024
    
    //  NAND COMMAND,      
    BYTE    XB_InfoReadFirstCmd;                //0xC000_1800 //1 0x00
    BYTE    XB_InfoReadLastCmd;                 //0xC000_1801 //1 0x30
    BYTE    XB_InfoMultiReadFirstCmd;           //0xC000_1802 //1 0x60
    BYTE    XB_InfoMultiReadLastCmd;            //0xC000_1803 //1 0x30
    BYTE    XB_InfoMultiCacheReadLastCmd;       //0xC000_1804 //1 0x33
    BYTE    XB_InfoCacheReadCmd;                //0xC000_1805 //1 0x31
    BYTE    XB_InfoCacheReadEndCmd;             //0xC000_1806 //1 0x3f
    BYTE    XB_InfoProgramFirstCmd;             //0xC000_1807 //1 0x80
    BYTE    XB_InfoDummyProgramCmd;             //0xC000_1808 //1 0x11
    BYTE    XB_InfoMultiProgramCmd;             //0xC000_1809 //1 0x81
    BYTE    XB_InfoProgramLastCmd;              //0xC000_180A //1 0x10
    BYTE    XB_InfoCacheProgramCmd;             //0xC000_180B //1 0x15
    BYTE    XB_InfoEraseFirstCmd;               //0xC000_180C //1 0x60
    BYTE    XB_InfoEraseLastCmd;                //0xC000_180D //1 0xD0
    BYTE    XB_InfoMultiEraseFirstCmd;          //0xC000_180E //1 0x60
    BYTE    XB_InfoMultiEraseLastCmd;           //0xC000_180F //1 0xD0
    BYTE    XB_InfoReadChipStatusCmd[4];        //0xC000_1810 //1*4   0xf1,0xf2,0xf3,0xf4
    BYTE    XB_InfoReadStatusCmd;               //0xC000_1814 //1 0x70
    BYTE    XB_InfoRandomInCmd;                 //0xC000_1815 //1 0x85    
    BYTE    XB_InfoONFIResetCmd;                //0xC000_1816 //1 0xfc
    BYTE    XB_InfoReadStatusEnhCmd;            //0xC000_1817 //1 0x78, 0x73

    //Global Var,0xF002_1000    0x1000~0x10ff
    WORD    XW_InfoInfoFBlock[2];               //0xC000_1818 //2*2
    WORD    XW_InfoISPFBlock[2];                //0xC000_181C //2*2
    BYTE    XB_InfoInfoFCE[2];                  //0xC000_1820 //1*2
    BYTE    XB_InfoISPFCE[2];                   //0xC000_1822 //1*2
    BYTE    XB_InfoBootFBlock[2];               //0xC000_1824 //2
    BYTE    XB_InfoBootFCE[2];                  //0xC000_1826 //2
    BYTE    XB_InfoFCE_Table[8];                //0xC000_1828 //8         //  8CE
    BYTE    XB_InfoLUNsPerCE;                   //0xC000_1830 //1         //(1~4)
    BYTE    XB_InfoPageSize;                    //0xC000_1831 //1         //??1k(1~16)    
    WORD    XW_InfoWBlockPerLUN;                //0xC000_1832 //2         //<16k  //記錄一個LUN 有多少SYS1WFBlock
    WORD    XW_InfoFBlockPerLUN;                //0xC000_1834 //2         //<16k  //記錄一個LUN 有多少SYS1FBlock
    WORD    XW_InfoPagePerBlock;                //0xC000_1836 //2         //預定最大1024
    BYTE    XB_InfoShiftForFBlock;              //0xC000_1838 //1         //LUN有多少可定址的Log-FBlock,2^n(0~14)
    BYTE    XB_InfoShiftForFPlane;              //0xC000_1839 //1         //Log-FBlock有多少FPlane,2^n(0~2)
    BYTE    XB_InfoShiftForFPage;               //0xC000_183A //1         //Log-FBlock有多少FPage,2^n(0~10)(192算256)
    BYTE    XB_InfoShiftForIntIntlv;            //0xC000_183B //1         //2^n(max3)
    BYTE    XB_InfoShiftForExtIntlv;            //0xC000_183C //1         //2^n(max3)
    BYTE    XB_InfoFlashID;                     //0xC000_183D //1         //vendor
    BYTE    XB_InfoFlashType;                   //0xC000_183E //1         //slc,mlc,tlc,qlc
    BYTE    XB_InfoTotalCENum;                  //0xC000_183F //1         //Max 8 CE
    WORD    XW_InfoSys1SpareBlock[32];          //0xC000_1840 //2*32      //boot*2 info*2 isp*2 Link*4 qtab*8 wpro*1
    BYTE    XB_Rsv1[64];                        //0xC000_1880 //2*32
    BYTE    XB_InfoSys1SprBlockCE[32];          //0xC000_18C0 //1*32
    BYTE    XB_Rsv25[32];                       //0xC000_18E0 //1*32
    BYTE    XB_InfoSys1BlkNum;                  //0xC000_1900 //1         //
    BYTE    XB_InfoSys1Mode;                    //0xC000_1901 //1         //bit 0 withbootcode,bit1 withSeedtab,bit2 remp,bit3 rm,bit4 Sys1Strong, bit5 Sys1RM
    BYTE    XB_InfoRTBBInSys1;                  //0xC000_1902 //1         //
    BYTE    XB_InfoSys2Mode;                    //0xC000_1903 //1         //
    BYTE    XB_InfoEccBit;                      //0xC000_1904 //1         // 13~72
    BYTE    XB_InfoEccMode;                     //0xC000_1905 //1         //bit0 Sel6/Sel10,bit 1 metaSeed,bit2 DataShaping,bit3 metaShaping          
    BYTE    XB_InfoSys1BootMode;                //0xC000_1906 //1
    BYTE    XB_InfoSys2Mode1;                   //0xC000_1907 //1
    WORD    XW_InfoTotalHBlock;                 //0xC000_1908 //2
    WORD    XW_InfoTotalFBlock;                 //0xC000_190A //2
    WORD    XW_InfoHBlockPerQMU;                //0xC000_190C //2 
    WORD    XW_InfoBootDataStartColAddr;        //0xC000_190E //2
    WORD    XW_InfoRandSeed[2];                 //0xC000_1910 //2*2
    WORD    XW_InfoFlashMetaStartColAddr;       //0xC000_1914 //2
    WORD    XW_InfoFlashDataStartColAddr;       //0xC000_1916 //2
    WORD    XW_InfoBootMetaStartColAddr;        //0xC000_1918 //2
    WORD    XW_InfoBootRandSeed;                //0xC000_191A //2
    BYTE    XB_InfoFlashOpt;                    //0xC000_191C //1         //bit0 multiread with 5ale
    BYTE    XB_InfoFlashIgnore;                 //0xC000_191D //1         //bit0 ignore erasefail, bit1 ignore programfail, bit2 
    BYTE    XB_InfoEccDataThValue;              //0xC000_191E //1  
    BYTE    XB_InfoEccMetaThValue;              //0xC000_191F //1

#if 0 //for ISP customize
    BYTE    XB_InfoT1BlkChkNum;                 //0xC000_1920 //1
    BYTE    XB_InfoMinTLCSprBlk1;               //0xC000_1921 //1
    BYTE    XB_InfoSlcWLDiffCnt;                //0xC000_1922 //1  
    BYTE    XB_InfoTlcWLDiffCnt;                //0xC000_1923 //1
    BYTE    XB_Rsv5;                            //0xC000_1924 //1
    BYTE    XB_Rsv6;                            //0xC000_1925 //1
    BYTE    XB_Rsv7;                            //0xC000_1926 //1
    BYTE    XB_Rsv8;                            //0xC000_1927 //1
    BYTE    XB_Rsv9;                            //0xC000_1928 //1
    BYTE    XB_Rsv10;                           //0xC000_1929 //1
    BYTE    XB_Rsv11;                           //0xC000_192A //1
    BYTE    XB_InfoPollingDelayTimer;           //0xC000_192B //1
    WORD    XW_InfoMvSeqPage;                   //0xC000_192C //2
    BYTE    XB_Rsv12;                           //0xC000_192E //1
    BYTE    XB_Rsv13;                           //0xC000_192F //1

    WORD    XW_Rsv14;                           //0xC000_1930 //2 
    BYTE    XB_InfoReadRetryNum;                //0xC000_1932 //1
    BYTE    XB_InfoEraseRetryNum;               //0xC000_1933 //1 
    BYTE    XB_Rsv15;                           //0xC000_1934 //1
    BYTE    XB_Rsv16;                           //0xC000_1935 //1
    BYTE    XB_InfoPerformanceOpt;              //0xC000_1936 //1
    BYTE    XB_Rsv17;                           //0xC000_1937 //1
    BYTE    XB_Rsv18;                           //0xC000_1938 //1
    BYTE    XB_Rsv19;                           //0xC000_1939 //1
    WORD    XW_InfoSleepMaxCount;               //0xC000_193A //2
    BYTE    XB_Rsv20[36];                       //0xC000_193C //1
    //BYTE    XW_ForceMarkBad[16],                //0xC000_1950
#else
    BYTE    XB_InfoMaxMapGrpNum;                //0xC000_1920 //1
    BYTE    XB_InfoMaxChildNumInOneGrp;         //0xC000_1921 //1
    BYTE    XB_InfoMaxFatNumInOneGrp;           //0xC000_1922 //1  
    BYTE    XB_InfoMaxQDataBlockNumInQGrp;      //0xC000_1923 //1
    BYTE    XB_InfoBoundaryMapGrpNum;           //0xC000_1924 //1
    BYTE    XB_InfoBoundaryQBlockNumInQGrp;     //0xC000_1925 //1
    BYTE    XW_InfoBoundaryQValiddataNum;       //0xC000_1926 //2
    BYTE    XB_InfoBoundarySpr2Num;             //0xC000_1928 //1
    BYTE    XB_InfoBoundaryFatNumInOneGrp;      //0xC000_1929 //1
    BYTE    XB_InfoWLSys2BlkEsCnt;              //0xC000_192A //1
    BYTE    XB_InfoPollingDelayTimer;           //0xC000_192B //1
    WORD    XW_InfoSlcAreaBlkNum;               //0xC000_192C //2
    BYTE    XW_InfoMvRandPage;                  //0xC000_192E //2

    WORD    XW_InfoDummyBusyDelayTimer;         //0xC000_1930 //2 
    BYTE    XB_InfoReadRetryNum;                //0xC000_1932 //1
    BYTE    XB_InfoEraseRetryNum;               //0xC000_1933 //1 
    BYTE    XB_InfoQuickFatValidNum;            //0xC000_1934 //1
    BYTE    XB_InfoQuickFatChkBlkNum;           //0xC000_1935 //1
    BYTE    XB_InfoPerformanceOpt;              //0xC000_1936 //1
    BYTE    XB_InfoReadRetryOpt;                //0xC000_1937 //1
    BYTE    XW_InfoMarkBadMvNum;                //0xC000_1938 //1
    WORD    XW_InfoSleepCount;                  //0xC000_193A //2
    WORD    XW_Rsv120;                          //0xC000_193C //2
    WORD    XW_InfoPageColNum;                  //0xC000_193E //2
    BYTE    XB_Rsv140[30];                      //0xC000_1940 //1
    //BYTE    XW_ForceMarkBad[16],                //0xC000_1950
    BYTE    XB_InfoSlcAccessCmd;                //0xC000_195E //1
    BYTE    XB_InfoSlcAbortCmd;                 //0xC000_195F //1
#endif    
    BYTE    XB_InfoCIDInfo[16];                 //0xC000_1960 //16
    BYTE    XB_InfoCSDInfo[16];                 //0xC000_1970 //16
    BYTE    XB_InfoMIDInfo[8];                  //0xC000_1980 //8
    LWORD   XL_InfoProtectedArea;               //0xC000_1988 //4
    LWORD   XL_InfoMKBSize;                     //0xC000_198C //4
    BYTE    XB_InfoFWOption[84];                //0xC000_1990 //??
    BYTE    XB_Rsv21;                           //0xC000_19E4 //1
    BYTE    XB_Rsv22;                           //0xC000_19E5 //1

    //add for SearchBoot
    BYTE    XB_InfoBootCodeLength[2];           //0xC000_19E6 //2, 1*2
    WORD    XW_InfoBootCodeSeed[2];             //0xC000_19E8 //4, 2*2
    WORD    XW_InfoBootCodeColAddr[2];          //0xC000_19EC //4, 2*2
    LWORD   XL_InfoBootCodeRowAddr[2];          //0xC000_19F0 //8, 4*2

    //Mark Boot Table Serial
    BYTE    XB_InfoMark[8];                     //0xC000_19F8 //8, 1*8
    
    //Strong Page Table 0xC000_1A00-0xC000_1BFF
    WORD    XW_InfoStrongPage[cStrongPageSize]; //0xC000_1A00 //2*256
    
} tBootInfo;

#ifndef WIN32
extern tBootInfo tInfoRAM;
#endif // WIN32
#endif
