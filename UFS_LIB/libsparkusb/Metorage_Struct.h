#ifndef METORAGE_STRUCT_H
#define METORAGE_STRUCT_H
#include "RomInfoStruct.h"

#define TRUE                                    1
#define FALSE                                   0

#define C_MAX_CE_CNT                            4
#define C_MAX_BLK_CNT                           1024
#define C_PRG_BUF_SIZE                          72*1024 //PRGA:8K PRGB:32K PRGC:32K
/*Memory address*/
#define C_PWR_RESET_VARS_ADDR                   0xF0000000
#define C_PWR_RESET_VARS_SIZE                   0x2000
#define C_GLOBAL_INFO_ADDR                      0xF0002000
#define C_GLOBAL_INFO_SIZE                      0x4000
#define C_STBY_CACHE_ADDR                       0xF0006000
#define C_STBY_CACHE_SIZE                       0x1D00
#define C_SMI_ROM_VARS_ADDR                     0xF0007D00
#define C_SMI_ROM_VARS_SIZE                     0x300
#define C_P2L_ADDR                              0xF0008000
#define C_P2L_MEM_SIZE                          0xA000 //10 * 4K
#define C_L1_ADDR                               0xF003C000
#define C_L2_ADDR                               0xF0038000
#define C_CACHE_BUF_ADDR                        0xF0040000
#define C_CACHE_BUF_SIZE                        0x1000

#define C_VPC_TBL_ADDR                          0xF0002000
#define C_VPC_TBL_SIZE                          0x1000
#define C_EC_TBL_ADDR                           0xF0003000
#define C_EC_TBL_SIZE                           0x800


/*sys1 related*/
#define ocB_SymbolChk                           cB_BIT3
#define mXb_Sys1Boot0FindErr                    (XB_bsts_Sys1FindErr&cB_BIT0)
#define mXb_SetSys1Boot0FindErr                 XB_bsts_Sys1FindErr|=cB_BIT0
#define mXb_Sys1Boot1FindErr                    (XB_bsts_Sys1FindErr&cB_BIT1)
#define mXb_SetSys1Boot1FindErr                 XB_bsts_Sys1FindErr|=cB_BIT1
#define mXb_Sys1Info0FindErr                    (XB_bsts_Sys1FindErr&cB_BIT2)
#define mXb_SetSys1Info0FindErr                 XB_bsts_Sys1FindErr|=cB_BIT2
#define mXb_Sys1Info1FindErr                    (XB_bsts_Sys1FindErr&cB_BIT3)
#define mXb_SetSys1Info1FindErr                 XB_bsts_Sys1FindErr|=cB_BIT3
#define mXb_Sys1ISP0FindErr                     (XB_bsts_Sys1FindErr&cB_BIT4)
#define mXb_SetSys1ISP0FindErr                  XB_bsts_Sys1FindErr|=cB_BIT4
#define mXb_Sys1ISP1FindErr                     (XB_bsts_Sys1FindErr&cB_BIT5)
#define mXb_SetSys1ISP1FindErr                  XB_bsts_Sys1FindErr|=cB_BIT5

/*Rebuild related*/
#define ocB_RebuildNoIdxBlk                     cB_BIT0
#define ocB_RebuildGIErr                        cB_BIT1
#define ocB_RebuilidPORFirstW                   cB_BIT2
#define ocB_RebuildRdFlashUnc                   cB_BIT3
#define ocB_RebuildRdFlashEmpty                 cB_BIT4
#define ocB_RebuildDumpGcDest                   cB_BIT5
#define ocB_RebuildWeakPrg                      cB_BIT6

//bRebuildSts2

#define ocB_RebuildRdFlashP0Unc                 cB_BIT0
#define ocB_RebuildRdFlashP1Unc                 cB_BIT1
#define ocB_RebuildRdFlashP0Empty               cB_BIT2
#define ocB_RebuildRdFlashP1Empty               cB_BIT3

#define C_MIN_DATA_DICNT                        0x400

#define mXb_RebuildNoIdxBlk                     (gstGLVars.bRebuildSts&ocB_RebuildNoIdxBlk)
#define mXb_SetRebuildNoIdxBlk                  (gstGLVars.bRebuildSts|=ocB_RebuildNoIdxBlk)
#define mXb_ClrRebuildNoIdxBlk                  (gstGLVars.bRebuildSts&=(~ocB_RebuildNoIdxBlk))

#define mXb_RebuildGIErr                        (gstGLVars.bRebuildSts&ocB_RebuildGIErr)
#define mXb_SetRebuildGIErr                     (gstGLVars.bRebuildSts|=ocB_RebuildGIErr)
#define mXb_ClrRebuildGIErr                     (gstGLVars.bRebuildSts&=(~ocB_RebuildGIErr))

#define mXb_RebuilidPORFirstW                   (gstGLVars.bRebuildSts&ocB_RebuilidPORFirstW)
#define mXb_SetRebuilidPORFirstW                (gstGLVars.bRebuildSts|=ocB_RebuilidPORFirstW)
#define mXb_ClrRebuilidPORFirstW                (gstGLVars.bRebuildSts&=(~ocB_RebuilidPORFirstW))

#define mXb_RebuildRdFlashUnc                   (gstGLVars.bRebuildSts&ocB_RebuildRdFlashUnc)
#define mXb_SetRebuildRdFlashUnc                (gstGLVars.bRebuildSts|=ocB_RebuildRdFlashUnc)
#define mXb_ClrRebuildRdFlashUnc                (gstGLVars.bRebuildSts&=(~ocB_RebuildRdFlashUnc))

#define mXb_RebuildRdFlashEmpty                 (gstGLVars.bRebuildSts&ocB_RebuildRdFlashEmpty)
#define mXb_SetRebuildRdFlashEmpty              (gstGLVars.bRebuildSts|=ocB_RebuildRdFlashEmpty)
#define mXb_ClrRebuildRdFlashEmpty              (gstGLVars.bRebuildSts&=(~ocB_RebuildRdFlashEmpty))

#define mXb_RebuildWeakPrg                      (gstGLVars.bRebuildSts&ocB_RebuildWeakPrg)
#define mXb_SetRebuildWeakPrg                   (gstGLVars.bRebuildSts|=ocB_RebuildWeakPrg)
#define mXb_ClrRebuildWeakPrg                   (gstGLVars.bRebuildSts&=(~ocB_RebuildWeakPrg))

#define mXb_RebuildDumpGcDest                   (gstGLVars.bRebuildSts&ocB_RebuildDumpGcDest)
#define mXb_SetRebuildDumpGcDest                (gstGLVars.bRebuildSts|=ocB_RebuildDumpGcDest)
#define mXb_ClrRebuildDumpGcDest                (gstGLVars.bRebuildSts&=(~ocB_RebuildDumpGcDest))
#define mXb_RebuildRdFlashP0Unc                 (gstGLVars.bRebuildSts2&ocB_RebuildRdFlashP0Unc)
#define mXb_SetRebuildRdFlashP0Unc              (gstGLVars.bRebuildSts2|=ocB_RebuildRdFlashP0Unc)
#define mXb_ClrRebuildRdFlashP0Unc              (gstGLVars.bRebuildSts2&=(~ocB_RebuildRdFlashP0Unc))

#define mXb_RebuildRdFlashP1Unc                 (gstGLVars.bRebuildSts2&ocB_RebuildRdFlashP1Unc)
#define mXb_SetRebuildRdFlashP1Unc              (gstGLVars.bRebuildSts2|=ocB_RebuildRdFlashP1Unc)
#define mXb_ClrRebuildRdFlashP1Unc              (gstGLVars.bRebuildSts2&=(~ocB_RebuildRdFlashP1Unc))

#define mXb_RebuildRdFlashP0Empty               (gstGLVars.bRebuildSts2&ocB_RebuildRdFlashP0Empty)
#define mXb_SetRebuildRdFlashP0Empty            (gstGLVars.bRebuildSts2|=ocB_RebuildRdFlashP0Empty)
#define mXb_ClrRebuildRdFlashP0Empty            (gstGLVars.bRebuildSts2&=(~ocB_RebuildRdFlashP0Empty))

#define mXb_RebuildRdFlashP1Empty               (gstGLVars.bRebuildSts2&ocB_RebuildRdFlashP1Empty)
#define mXb_SetRebuildRdFlashP1Empty            (gstGLVars.bRebuildSts2|=ocB_RebuildRdFlashP1Empty)
#define mXb_ClrRebuildRdFlashP1Empty            (gstGLVars.bRebuildSts2&=(~ocB_RebuildRdFlashP1Empty))

#define M_IS_INVALID_SEGNO(x)                   ((x) == 0xFFFF)
#define M_IS_L2_SEGNO(x)                        (((x) != 0xFFFF) && (x >= 0xF000))
#define M_IS_L1_SEGNO(x)                        (((x) != 0xFFFF) && (x < 0xF000))




#define C_RBL_FFBLK 1
#define C_RBL_SUPER_BLK 0

/*ERASE*/
//#define mRb_SetWP_ERASE_SKIP                  RB_SD[rcB_CPU_SET_CSR1]|=cB_BIT7

/*Block management related*/

#define C_MAX_BLK_CNT                           1024
#define INVALID_SUPER_BLK                       0xFFFF
#define INVALID_VPC                             0xFFFFFFFF

#ifdef WL_DEBUG
#define C_HEC_THRESHOLD                         15
#define C_SLC_HEC_THRESHOLD                     15
#define C_WL_THRESHOLD                          20
#else
#define C_HEC_THRESHOLD                         75
#define C_SLC_HEC_THRESHOLD                     75
#define C_WL_THRESHOLD                          100
#endif
#define C_MAX_WL_Q_CNT                          4
#define C_MAX_SLC_FREEQ_NUM                     128
#define C_MAX_TLC_FREEQ_NUM                     32
#define C_CURRENT_SLC_FREEQ_NUM                 64
#define C_CURRENT_TLC_FREEQ_NUM                 32
#define C_TLC_FREE_THRESHOLD 3
#define C_SLC_FREE_THRESHOLD 6
#define C_INVALID_VPC_ENTRY 0xFFFFFFFF
#define C_SLCBLK_FPAGENUM 384*2*tInfoRAM.XB_InfoTotalCENum
#define C_MIN_GOODBLOCK_NUM                     942
#define C_USERAREABLOCK_NUM                     820
#define C_MAX_PSR_FREEQ_NUM                     8

#define C_BLKSTS_DATA              cB_BIT0
#define C_BLKSTS_SLC               cB_BIT1
#define C_BLKSTS_UNMAP             cB_BIT2
#define C_BLKSTS_POR               cB_BIT3
#define C_BLKSTS_INTERNAL_TLC      cB_BIT4
//#define C_BLKSTS_ERASED cB_BIT4
//#define C_BLKSTS_NEED_ERASE BIT5
#define C_BLKSTS_RBT               cB_BIT5
#define C_BLKSTS_RTBB_P0           cB_BIT6
#define C_BLKSTS_RTBB_P1           cB_BIT7
#define C_BLKSTS_RTBB             (cB_BIT6 | cB_BIT7)

#define M_IS_DATA_BLK(x)                            (gubBlkStsTable[x] & C_BLKSTS_DATA)
#define M_IS_SLC_BLK(x)                             (gubBlkStsTable[x] & C_BLKSTS_SLC)
#define M_IS_UNMAP_BLK(x)                           (gubBlkStsTable[x] & C_BLKSTS_UNMAP)
#define M_IS_POR_BLK(x)                             (gubBlkStsTable[x] & C_BLKSTS_POR)
#define M_IS_INTERNAL_TLC_BLK(x)                    (gubBlkStsTable[x] & C_BLKSTS_INTERNAL_TLC)

#define M_SET_DATA_BLK(x)                           (gubBlkStsTable[x] |= C_BLKSTS_DATA)
#define M_SET_SLC_BLK(x)                            (gubBlkStsTable[x] |= C_BLKSTS_SLC)
#define M_SET_UNMAP_BLK(x)                          (gubBlkStsTable[x] |= C_BLKSTS_UNMAP)
#define M_SET_POR_BLK(x)                            (gubBlkStsTable[x] |= C_BLKSTS_POR)
#define M_SET_INTERNAL_TLC_BLK(x)                   (gubBlkStsTable[x] |= C_BLKSTS_INTERNAL_TLC)

#define M_CLR_DATA_BLK(x)                           (gubBlkStsTable[x] &= ~C_BLKSTS_DATA)
#define M_CLR_SLC_BLK(x)                            (gubBlkStsTable[x] &= ~C_BLKSTS_SLC)
#define M_CLR_UNMAP_BLK(x)                          (gubBlkStsTable[x] &= ~C_BLKSTS_UNMAP)
#define M_CLR_POR_BLK(x)                            (gubBlkStsTable[x] &= ~C_BLKSTS_POR)
#define M_CLR_INTERNAL_TLC_BLK(x)                   (gubBlkStsTable[x] &= ~C_BLKSTS_INTERNAL_TLC)

#define C_HOST_OPEN_BID 0x00
#define C_TEMP_BID 0x01
#define C_MAP_BID 0x02
#define C_GC_OPEN_BID 0x03
#define C_SEQ_OPEN_BID 0x04
#define C_PSR_BID 0x05
#define C_DFX_BID 0x0D
#define C_IDX_BID 0x0E
#define C_BIB_BID 0x0F
#define C_GI_MARK 0x9527
#define C_GI_POR 0xC

/*L2P information*/
#define C_L1_CACHE_CNT                          8
#define C_L2_CACHE_CNT                          8
#define C_EXT_L1_CACHE_CNT                      16
#define C_SEQ_L1_CACHE_CNT                      12
#define C_SEQ_L2_CACHE_CNT                      4
#define C_DEFAULT_L2P_MODE                       0
#define C_SEQ_L2P_MODE                           1
#define C_EXT_L2P_MODE                           2

#define EXT_L2P_R                               cB_BIT0
#define EXT_L2P_W                               cB_BIT1
#define C_L1_ENTRY_CNT                          512
#define C_L1_ENTRY_SHIFT                        9
#define C_L1_ENTRY_MASK                         (C_L1_ENTRY_CNT - 1)
#define C_L2_ENTRY_CNT                          512
#define C_L3_ENTRY_CNT                          128
#define C_L2_ENTRY_SHIFT                        9
#define C_L2_ENTRY_MASK                         (C_L2_ENTRY_CNT - 1)
#define C_MAX_MAP_BLK_NUM                       16
#define C_INVALID_CACHE_IDX                     0xFF

#define C_L1_CACHE_DIRTY cB_BIT0
#define C_L1_CACHE_BUSY cB_BIT1
#define C_L1_CACHE_READY cB_BIT2
#define C_L1_CACHE_PEND_VPC cB_BIT3

#define C_L2_CACHE_DIRTY cB_BIT0
#define C_L2_CACHE_BUSY cB_BIT1
#define C_L2_CACHE_READY cB_BIT2

#define C_L2P_PRG_DONE  0
#define C_L2P_PRG_ONGOING 1
#define C_L2P_PRG_FAILED 2

#define C_MAX_MAP_GC_TH C_MAX_MAP_BLK_NUM - 2

//#define C_L2_ENTRY_SHIFT              9
//#define C_L2_ENTRY_CNT                (1 << C_L2_ENTRY_SHIFT)    //512
//#define C_L2_ENTRY_MASK               (C_L2_ENTRY_CNT - 1)
#define C_INVALID_L2P_ENTRY           0xFFFFFFFF
#define C_L2P_ENTRY_BLK_IDX_SHIFT 26
#define C_Page2kShift              3
#define C_L2_HEADER_MARK 0xF000


/*P2L information*/
#define C_OPEN_P2L_CACHE_CNT                    10
#define C_P2L_CACHE_CNT                         11
#define C_HOST_P2L_IDX                          0
#define C_GC_P2L_IDX                            1
#define C_SEQ0_P2L_IDX                          2
#define C_SEQ1_P2L_IDX                          3
#define C_SEQ2_P2L_IDX                          4
#define C_SEQ3_P2L_IDX                          5
#define C_SEQ4_P2L_IDX                          6
#define C_SEQ5_P2L_IDX                          7
#define C_SEQ6_P2L_IDX                          8
#define C_SEQ7_P2L_IDX                          9
#define C_GC_SRC_IDX                            10
#define C_P2L_ENTRY_CNT                         1024
#define C_P2L_ENTRY_CNT_MASK                    1023 //1024 - 1
#define C_P2L_CACHE_DIRTY                       cB_BIT0
#define C_P2L_NUM_PER_BLOCK_PER_CE              9
#define C_P2L_SIZE_BYTE                         4096

/*Host write related*/
#define C_TOTAL_512B_PER_MPAGE                  64
#define C_TOTAL_512B_PER_MPAGE_SHIFT            6
#define C_TOTAL_1K_PER_MPAGE                    32
#define C_TOTAL_1K_PER_MPAGE_SHIFT              5
#define C_TOTAL_4K_PER_MPAGE                    8
#define C_TOTAL_4K_PER_MPAGE_SHIFT              3

#define C_TOTAL_512B_PER_HALF_MPAGE             32
#define C_TOTAL_1K_PER_HALF_MPAGE               16
#define C_TOTAL_4K_PER_HALF_MPAGE               4

#define C_MAX_INTERLEAVE_LEVEL                  2
#define C_ENTRY_SIZE_512B                       8
#define C_ENTRY_MASK_512B                       0x7
#define C_ENTRY_SIZE_1K                         4
#define C_ENTRY_MASK_1K                         0x3

#define C_BUF_SIZE_SECTOR                       0x80
#define C_BUF_SIZE_SECTOR_SHIFT                 7
#define C_BUF_HALF_SIZE_SECTOR                  0x40
#define C_BUF_HALF_SIZE_SECTOR_SHIFT            6

#define C_BUF_POS_FOR_FLUSH_P2L_1ST             (C_BUF_HALF_SIZE_SECTOR - C_ENTRY_SIZE_512B)
#define C_BUF_POS_FOR_FLUSH_P2L_2ND             (C_BUF_SIZE_SECTOR - C_ENTRY_SIZE_512B)

#define C_P2L_ENTRY_VALUE_MASK                  0x3FFFFFFF
#define C_INVALID_L4K                           0xFFFFFFFF
#define C_INVALID_L2P_ENTRY                     0xFFFFFFFF
#define C_INVALID_P2L_ENTRY                     0xFFFFFFFF

#define C_L1_ENTRY_SHIFT                        9

//flush host p2l related
#define C_TOTAL_4K_PER_SLC_BLOCK_PER_CE         0xC00
#define C_TOTAL_4K_PER_TLC_BLOCK_PER_CE         0x2400

#define C_FLUSH_P2L_GAP                         (1024) //4KB P2L
//#define M_CHK_DO_FLUSH_P2L_HCLPTR(x)            (((x == BUF_POS_FOR_FLUSH_P2L_1ST) || (x == BUF_POS_FOR_FLUSH_P2L_2ND)) ? TRUE: FALSE)
//#define M_CHK_DO_FLUSH_P2L_PTR4K(x)             (((P2L_ENTRY_CNT - (x & P2L_ENTRY_MASK)) == TOTAL_4K_PER_MPAGE) ? TRUE: FALSE)

#define M_CHK_SLC_BLK_FULL(x)                   ((x >= gstGIVars.stDevConfig.wTotalEntryPerSlcBlock) ? TRUE: FALSE)
#define M_GET_BUF_BOUNDARY(x)                   ((x >= C_BUF_HALF_SIZE_SECTOR) ? C_BUF_SIZE_SECTOR: C_BUF_HALF_SIZE_SECTOR)
#define M_CHK_DO_FLUSH_P2L_PTR4K(x)             ((((C_P2L_ENTRY_CNT - ((x) & C_P2L_ENTRY_CNT_MASK)) == C_TOTAL_4K_PER_MPAGE) || \
        (((C_TOTAL_4K_PER_SLC_BLOCK_PER_CE * gstGIVars.stDevConfig.bCENum) - (x)) == C_TOTAL_4K_PER_MPAGE)) ? TRUE: FALSE)
#define M_CHK_DO_FLUSH_P2L_HOSTPTR(x)           (((x == C_BUF_POS_FOR_FLUSH_P2L_1ST) || (x == C_BUF_POS_FOR_FLUSH_P2L_2ND)) ? TRUE: FALSE)
#define M_GET_FLUSH_P2L_CHK_PTR(x)              ((x < C_BUF_HALF_SIZE_SECTOR) ? (C_BUF_POS_FOR_FLUSH_P2L_1ST - 1): (C_BUF_POS_FOR_FLUSH_P2L_2ND - 1))

#define C_TRIG_FIRST_512B   1
#define C_TRIG_LAST_512B    2
#define C_TRIG_1K           3

/*Host Read related*/

//ENABLE OR DISABLE FUNCTION
//#define CACHE_READ_ENABLE
#define READ_ERR_HANDLE
//#define HOST_BEHAVIOR_RECORD
//#define HOST_DATA_CACHED
#define ENABLE_GC_READVERIFY
#define ENABLE_INTERNAL_COPYBACK
//#define WR_FLOW_VER2
//#define ENABLE_PSR   //PSR:PROTECTED_SLC_REGION
//#define FAKE_CODEWORD_EN
#define ENABLE_WEARLEVELING

//FUNCTION
#define GET_MIN(x, y)               (((x) > (y))? (y): (x))

//INVALID TYPE
#define INVALID_BYTE  0XFF
#define INVALID_WROD  0XFFFF
#define INVALID_DWROD 0XFFFFFFFF

//LOAD TABLE RELATED
#define TYPE_L1_TABLE (0)
#define TYPE_L2_TABLE (1)
#define INVALID_TABLE (0XFF)

//PAGE RELATED
#define C_TOTAL_4K_PER_BIND_BLOCK     (C_TOTAL_4K_PER_MPAGE * C_TOTAL_CE)
#define C_TOTAL_SECTOR_PER_BIND_BLOCK (C_TOTAL_512B_PER_MPAGE * C_TOTAL_CE)
#define C_SIZE_SECTOR                 (0X200)

//READQ RELATED
#define MAX_READ_CMDQ   (0X10)
#define TOTAL_CMDQ_IDX  (0X100)
#define CMDQ_SHIFT      (0X10)
#define INDEX_INC       (0x02)
#define INDEX_CLEAR     (0x03)

//CMDQ RELATED
#define CMDQ_NEED_CNT_MULTIPLANE_READ    12
#define CMDQ_NEED_CNT_SINGLEPLANE_READ   6
#define CMDQ_NEED_CNT_MULTIPLANE_DMAOUT  12
#define CMDQ_NEED_CNT_SINGLEPLANE_DMAOUT 6
#define CMDQ_NEED_CNT_TRIGER_DATA        3
#define CMDQ_NEED_CNT_TRIGER_DUMMY       1
#define CMDQ_NEED_CNT_SET_SEED           1
#define CMDQ_NEED_CNT_SET_MODE           1
#define CMDQ_NEED_CNT_SET_DELAY          1
#define CMDQ_NEED_CNT_SET_CE_ON          1
#define CMDQ_NEED_CNT_SET_CE_OFF         1
#define CMDQ_NEED_CNT_SET_CLE            1
#define CMDQ_NEED_CNT_READ_STATUS        2
#define CMDQ_NEED_CNT_IDX_OPTION         1

//SD RELATED
#define MAX_AUTO_TRAN_CNT   0XFF

//DEVICE RELATED
#define C_TOTAL_CE          gstGIVars.stDevConfig.bCENum

/*Read retry related*/
#define TLC_CLOSE_BLOCK_VTH_GROUP_SIZE  16
#define TLC_OPEN_BLOCK_VTH_GROUP_SIZE   14
#define ENTRY_PER_TLC_VTH_GROUP         7

#define SLC_VTH_GROUP_SIZE              5
#define ENTRY_PER_SLC_VTH_GROUP         1
#define TYPE_HARD_DECODE                1
#define TYPE_SOFT_DECODE                0
#define INVALID_DECODE_TYPE             0XFF

#define HARD_DECODE_VAR 180
#define SOFT_DECODE_VAR 250
#define DECODE_FAIL     400

#define DELAY_TICK_1000 1000
#define DELAY_TICK_300  300
#define DELAY_TICK_100  100
#define DELAY_TICK_50   50
#define DELAY_TICK_30   30
#define DELAY_TICK_10   10
#define DELAY_TICK_5    5

#define BUF_SEL_TSB      0
#define BUF_SEL_PRG      1

#define TYPE_OPEN_BLOCK  0
#define TYPE_CLOSE_BLOCK 1

#define RETRY_TYPE_DATA  cB_BIT1
#define RETRY_TYPE_MATE  cB_BIT0

#define E_CMDQ_DONE      0
#define E_ECC_ERROR      1
#define E_CMDQ_TIMEOUT   2
#define E_HOST_STOP      3

#define E_PUSH_ERROR     1
#define E_PUSH_DONE      0
#define TABLE_SIZE_1K    2

#define SEED_TABLE_MASK  0X3FF

#define rcB_MAX_ITERATION_USED_IN_BF_MODE 0x48
#define rcB_MAX_ITERATION_USED_IN_N6_MODE 0x58
#define rcB_LDPC_DECODER_SETTING2         0x64

#define M_GetEccErrMap()        GET_REG_UW(RW_BankNFC, (rcB_NFC_RTRY_IDX0 / 2))
#define M_GetEccErrMap1()       GET_REG_UW(RW_BankNFC, (rcB_NFC_RTRY_IDX2 / 2))

#define M_SetMaxIter_BF(x)      (RB_LDPCINTNAL[rcB_MAX_ITERATION_USED_IN_BF_MODE]=(x))
#define M_SetN6MaxIter(x)       (RB_LDPCINTNAL[rcB_MAX_ITERATION_USED_IN_N6_MODE]=(x))
#define M_SetHardDecodeMode(x)  (RB_LDPCINTNAL[rcB_LDPC_DECODER_SETTING2] = ((RB_LDPCINTNAL[rcB_LDPC_DECODER_SETTING2] & 0xFC) | x));
#define M_CHK_SynWeight()       (((RB_BankNFC[rcB_LDPC_CWRSLT_H] & 0x07) << 8) | RB_BankNFC[rcB_LDPC_CWRSLT_L])

#define M_CHK_ECCFail           (RB_BankNFC[rcB_NFC_ECC_STATUS] & (cB_BIT0 | cB_BIT1 | cB_BIT4))   //data or spare ecc fail
#define M_CHK_MetaECCFail       (RB_BankNFC[rcB_NFC_ECC_STATUS] & cB_BIT1)      //meta ecc fail
#define M_CHK_DataECCFail       (RB_BankNFC[rcB_NFC_ECC_STATUS] & cB_BIT0)      //data ecc fail


#define M_SetDecMode_Bypass     (RB_BankNFC[rcB_NFC_ECC_CTRL] &= ~cB_BIT0) //close ECC
#define M_ClrDecMode_Bypass     (RB_BankNFC[rcB_NFC_ECC_CTRL] |= cB_BIT0) //enable ECC
#define M_SetLDPCMode(x)        {RB_BankNFC[rcB_LDPC_MODE] = x; RB_BankNFC[rcB_LDPC_CTRL1] |= 0xE0;RB_BankNFC[rcB_LDPC_CTRL1] &= ~0xE0;}
#define M_SetLDPCSoftMode(x)    {RB_BankNFC[rcB_LDPC_SOFT_MODE] = x;}
#define M_SetLDPCSoftIdx(x)     {RB_BankNFC[rcB_LDPC_DEC_INFO] = ((RB_BankNFC[rcB_LDPC_DEC_INFO] & 0xF8)| x);}
#define M_SetLDPCSoftLastDMA(x) {RB_BankNFC[rcB_LDPC_DEC_INFO] = ((RB_BankNFC[rcB_LDPC_DEC_INFO] & 0x7F)| (x << 7));}
#define M_EnLDPCClearDSP        RB_BankNFC[rcB_LDPC_CTRL1] |= 0xE0;
#define M_DisLDPCClearDSP       RB_BankNFC[rcB_LDPC_CTRL1] &= ~0xE0;
#define M_EN_ScrbBoth           mRb_SetDataShap;
#define M_DIS_ScrbBoth          mRb_ClrDataShap;
#define M_ChkQueDone            {WaitAutoCmdqFinish(); }
#define SysDelay(x)             {AutoDelay(x); M_ChkQueDone;}

#define mRb_EccClrDiCnt                         (RB_BankNFC[rcB_NFC_ECC_CTRL]|=cB_BIT4)
#define mRb_EccDiCnt1En                         (RB_BankNFC[rcB_NFC_ECC_CTRL]|=(cB_BIT5 + cB_BIT6))
#define mRb_EccDiCnt1Disable                    (RB_BankNFC[rcB_NFC_ECC_CTRL]&=~(cB_BIT5 + cB_BIT6))

#define rcB_ECC_DI_THR_L                        0x50
#define rcB_ECC_DI_THR_M                        0x51
#define mRb_DiCntErr                            (RB_BankNFC[rcB_NFC_ECC_STATUS]&0x08)

#define M_CmdqNearFullIntEn                     (RB_GlobalNFC[rcB_INTE0] |= cB_BIT4)
#define M_CmdqNearFullIntDis                    (RB_GlobalNFC[rcB_INTE0] &= ~cB_BIT4)
#define M_CmdqPauseIntEn                        (RB_GlobalNFC[rcB_INTE0] |= cB_BIT0)
#define M_CmdqPauseIntDis                       (RB_GlobalNFC[rcB_INTE0] &= ~cB_BIT0)


#define M_ClrCmdqNearFullInt                    (RB_GlobalNFC[rcB_INTS0] = cB_BIT4)
#define M_ClrCmdqPauseInt                       (RB_GlobalNFC[rcB_INTS0] = cB_BIT0)

#define M_CmdqUncPauseEn                        (RB_BankNFC[rcB_NFC_CMDQ_PAUSE_EN] |= cB_BIT0)
#define M_CmdqUncPauseDis                       (RB_BankNFC[rcB_NFC_CMDQ_PAUSE_EN] &= ~cB_BIT0)
#define M_ClrCmdqOverflow                       (RB_BankNFC[rcB_NFC_CMDQ_STATUS] |= cB_BIT4)
#define M_CmdqNearFull                          (RB_BankNFC[rcB_NFC_CMDQ_STATUS] & cB_BIT5)

#define M_CmdqOverflow()                        (RB_BankNFC[rcB_NFC_CMDQ_STATUS] & cB_BIT4)
#define M_CmdqFull()                            (RB_BankNFC[rcB_NFC_CMDQ_STATUS] & cB_BIT6)
#define M_CmdqPauseInt()                        (RB_GlobalNFC[rcB_INTS0] & cB_BIT0)
#define M_CmdqNearFullInt()                     (RB_GlobalNFC[rcB_INTS0] & cB_BIT4)

#define ClrPauseSts()                           (RB_BankNFC[rcB_NFC_CMDQ_CTRL] |= cB_BIT1)
#define IsForcePause()                          (RB_BankNFC[rcB_NFC_CMDQ_PAUSE_STATUS] & cB_BIT7)
#define M_ChkInReadFlow()                       (gstGIVars.stReadPara.bInReadFlow)

//bit[5:4] flash_type
//00: SDR mode
//01: Toggle mode
//10: ONFI sync mode
//11: ONFI NV_DDR2 mode
//Toggle & NV_DDR2 set feature need 8byte(rising & falling set the same byte)
#define M_ChkIsToggleMode                       ((RB_GlobalNFC[rcB_NFC_CTRL2] & 0x10) == 0x10)

/*end of Read retry related*/

//XB_bsts_WrFlag1 should be reset after power-on or sleeping
#define mXb_HostDataCached                                  (XB_bsts_WrFlag1 & cB_BIT0)
#define mXb_SetHostDataCached                               (XB_bsts_WrFlag1 |= cB_BIT0)
#define mXb_ClrHostDataCached                               (XB_bsts_WrFlag1 &= ~cB_BIT0)
#define mXb_HostWriteLoopExitI                              (XB_bsts_WrFlag1 & cB_BIT1)
#define mXb_SetHostWriteLoopExitI                           (XB_bsts_WrFlag1 |= cB_BIT1)
#define mXb_ClrHostWriteLoopExitI                           (XB_bsts_WrFlag1 &= ~cB_BIT1)
#define mXb_HostWriteLoopExitII                             (XB_bsts_WrFlag1 & cB_BIT2)
#define mXb_SetHostWriteLoopExitII                          (XB_bsts_WrFlag1 |= cB_BIT2)
#define mXb_ClrHostWriteLoopExitII                          (XB_bsts_WrFlag1 &= ~cB_BIT2)
#define mXb_HostWriteFlushP2lStepI                          (XB_bsts_WrFlag1 & cB_BIT3)
#define mXb_SetHostWriteFlushP2lStepI                       (XB_bsts_WrFlag1 |= cB_BIT3)
#define mXb_ClrHostWriteFlushP2lStepI                       (XB_bsts_WrFlag1 &= ~cB_BIT3)
#define mXb_HostWriteFlushP2lStepII                         (XB_bsts_WrFlag1 & cB_BIT4)
#define mXb_SetHostWriteFlushP2lStepII                      (XB_bsts_WrFlag1 |= cB_BIT4)
#define mXb_ClrHostWriteFlushP2lStepII                      (XB_bsts_WrFlag1 &= ~cB_BIT4)
#define mXb_HostWriteJustStart                              (XB_bsts_WrFlag1 & cB_BIT5)
#define mXb_SetHostWriteJustStart                           (XB_bsts_WrFlag1 |= cB_BIT5)
#define mXb_ClrHostWriteJustStart                           (XB_bsts_WrFlag1 &= ~cB_BIT5)
#define mXb_HostWriteEraseMode                              (XB_bsts_WrFlag1 & cB_BIT6)
#define mXb_SetHostWriteEraseMode                           (XB_bsts_WrFlag1 |= cB_BIT6)
#define mXb_ClrHostWriteEraseMode                           (XB_bsts_WrFlag1 &= ~cB_BIT6)
#define mXb_HostWriteFlushP2lStepIII                        (XB_bsts_WrFlag1 & cB_BIT7)
#define mXb_SetHostWriteFlushP2lStepIII                     (XB_bsts_WrFlag1 |= cB_BIT7)
#define mXb_ClrHostWriteFlushP2lStepIII                     (XB_bsts_WrFlag1 &= ~cB_BIT7)

//XB_bts_GlobalFlag1 used for extra function
#define mXb_PSRWrite                                        (XB_bts_GlobalFlag1 & cB_BIT0)
#define mXb_SetPSRWrite                                     (XB_bts_GlobalFlag1 |= cB_BIT0)
#define mXb_ClrPSRWrite                                     (XB_bts_GlobalFlag1 &= ~cB_BIT0)
#define mXb_WriteCrossBoundary                              (XB_bts_GlobalFlag1 & cB_BIT1)
#define mXb_SetWriteCrossBoundary                           (XB_bts_GlobalFlag1 |= cB_BIT1)
#define mXb_ClrWriteCrossBoundary                           (XB_bts_GlobalFlag1 &= ~cB_BIT1)
#define mXb_PSRFlushRemainedData                            (XB_bts_GlobalFlag1 & cB_BIT2)
#define mXb_SetPSRFlushRemainedData                         (XB_bts_GlobalFlag1 |= cB_BIT2)
#define mXb_ClrPSRFlushRemainedData                         (XB_bts_GlobalFlag1 &= ~cB_BIT2)
#define mXb_PSRAlreadyFlushRemainedData                     (XB_bts_GlobalFlag1 & cB_BIT3)
#define mXb_SetPSRAlreadyFlushRemainedData                  (XB_bts_GlobalFlag1 |= cB_BIT3)
#define mXb_ClrPSRAlreadyFlushRemainedData                  (XB_bts_GlobalFlag1 &= ~cB_BIT3)

//DEBUG_TIMER_ENABLE
//#define READ_DEBUG_TIMER_ENABLE
//UART PRINT
//#define READ_DEBUG_PRINT
//#define WRITE_DEBUG_PRINT
#define RETRY_DEBUG_PRINT
#define REBUILD_DEBUG_PRINT
#define DFX_DEBUG_PRINT
#define HIGH_LEVEL_ERROR_LOG
//#define INT_DEBUG_PRINT

#define UART_PRINT_SHADOW
#ifdef  UART_PRINT_SHADOW
#define Print(...)          CONSOL_Printf(__VA_ARGS__)
#define Str(s)              #s
#define Print_Str(s)        Print(Str(s)##"\r\n")
#define Print_Func()        Print("%s\r\n", __FUNCTION__)
#define Print_Var(x)        Print(Str(x)##"=0x%x\r\n", (x))
#define Print_Int(x)        Print(Str(x)##"=%d\r\n", (x))
#define Print_Reg(r)        Print("0x%x=0x%x\r\n", &##r, r)
#define Print_Line()        Print("%d\r\n", __LINE__)
#endif

//DFX
#define ENABLE_DFX_LOG
#define DFX_UART_OUTPUT     1
//#define DFX_CMD_EN
#define DFX_MSG_BLK_SHIFT   16
#define DFX_MSG_PACK_BLK(x) ((LWORD)(x) << DFX_MSG_BLK_SHIFT)

#define DFX_MSG_SIZE        0x08
#define DFX_LOG_SIZE        0xC00

#define TYPE_DFX_DATA       0XAA
#define TYPE_DFX_TABLE      0XAC

#define MAX_DFX_BLOCK_CNT   4//0x5C0 // (3K - 128b  -> 0xb80) / 2
#define MIN_DFX_BLOCK_CNT   2
#define MAX_PAGE_CNT        C_SLCBLK_FPAGENUM
#define DFX_ADDR            0xF0007000
#define DFX_PROG_BUF_POS    ocB_ProgBuf13
#define DFX_BUF_POS         ocB_ProgBuf1C
#define DFX_CE_MASK         (tInfoRAM.XB_InfoTotalCENum - 1)
#define DFX_PLANE_MASK      0x01

#define DFX_PRINT_STR(s)         Print(Str(s))
#define DFX_PRINT_SPACE()        Print(" ")
#define DFX_PRINT_CHANGE_LINE()  Print("\r\n")


#define DFX_PRINT_MSG(t, op, code)  \
    {   \
        DFX_PRINT_STR(t);       DFX_PRINT_SPACE();\
        DFX_PRINT_STR(op);      DFX_PRINT_SPACE();\
        DFX_PRINT_STR(code);    DFX_PRINT_SPACE();\
        DFX_PRINT_CHANGE_LINE();\
    }

#define DFX_ADD_MSG(t, op, code) \
    {   \
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.stMsg.bType   = t;\
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.stMsg.bOpCode = op;\
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.l[1] = code;\
        I206_DML_DfxPushMsg();\
    }

#define DFX_ADD_MSG1(t, op, codew, codel) \
    {   \
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.stMsg.bType   = t;\
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.stMsg.bOpCode = op;\
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.w[1] = codew;\
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.l[1] = codel;\
        I206_DML_DfxPushMsg();\
    }

#define DFX_ERR_CODE(code, stop) \
    {   \
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.stMsg.bType   = E_DFX_ERR;\
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.stMsg.bOpCode = 0;\
        gstGLVars.stDfxRunTimeInfo.uDfxMsg.l[1] = code;\
        I206_DML_DfxPushMsg();\
        while(stop);\
    }


typedef enum
{
    E_L2ENTRY_NOR = 0,
    E_L2ENTRY_PEND_VPC,
    E_L2ENTRY_RSV,
    E_L2ENTRY_UNKNOW
} E_L2_ENTRY_STS;


typedef union
{
    struct
    {
        LWORD Offset2k : 26; // 2k offset within a map block
        LWORD BlkIdx : 4; //map block index
        LWORD sts : 2; // 00: normal, 01: pending VPC, 10: rsv, 11: unmap
    } bits;

    LWORD dw;

} ST_L2L3_ENTRY;


typedef struct //20 byte
{
    BYTE barBankProgSts[C_MAX_CE_CNT];

    LWORD lwCurrL4K;
    BYTE bBlkSn;

    BYTE bUpdatePageCnt;
    BYTE bL2CacheIdx;
    BYTE bCurrChkIdx;
    BYTE bCurrIdx;

    BYTE bCurrChkBank;
    BYTE bWrGcFlag;
    BYTE bHandleCacheData;
    WORD wBackUpWrPtr;
    WORD wPSRBackUpWrPtr;
    LWORD lwNonAlignUNCL4k[2];
    LWORD lwBackUpHostXfrCnt;
} ST_WRITE_RELATED_VAR;

typedef struct
{
    LWORD lwL4K[C_TOTAL_4K_PER_MPAGE];
    BYTE bCachedPtr;
    BYTE bValidL4K;
} ST_WRITE_CACHE_INFO;

#define BUF_STATE_NONE          0
#define BUF_STATE_HOST_TRIG     1
#define BUF_STATE_FLASH_TRIG    2
typedef struct
{
    BYTE bHostTrigPtr;
    BYTE bFlashTrigPtr;
    BYTE bBufState[2]; //0 empty, 1 host side triggered, 2 flash side triggered
#ifdef WR_FLOW_VER2
    BYTE bBufIdx[2];
#endif
} ST_WRITE_BUF_CONSOLE;

typedef struct
{
    LWORD wBlkNo;
    LWORD wPagePtr;
    LWORD wLastGIPos;
    LWORD wLastECBLPos;
    BYTE bChgBlk;
} ST_BIB_INFO;

typedef struct
{
    WORD wSegNo[8];
} ST_L2P_BLK_HEADER;

typedef struct
{
    BYTE bBlkId;
    WORD wECCnt;
    WORD wBIBBlkNo;
} ST_INDEX_BLK_HEADER;

typedef struct
{
    BYTE bBlkId;
    BYTE bType;
    WORD wGIPos;
} ST_BIB_BLK_HEADER;

#define C_GC_RV_FAIL            0x80000000
#define C_SRC_DATA_UNC          0x40000000

#define C_UNC_L4K               0xFFFFFFFC
#define C_DUMMY_L4K             0xFFFFFFFD
#define C_L1_ENTRY_BLK_SHIFT    16
#define C_L1_ENTRY_4K_MASK      (0x0000FFFF)

//L1 entry status
#define C_L1_ENTRY_NORMAL     0
#define C_L1_ENTRY_SLC        1
#define C_L1_ENTRY_UNC        2
#define C_L1_ENTRY_UNKNOW     3
typedef union
{
    struct
    {
    LWORD Offset4k :
        C_L1_ENTRY_BLK_SHIFT; // 4k offset within a super block
        LWORD BlkNo : 14; //Super block no
        LWORD Sts : 2; //00: normal, 01: slc, 10: UNC. 11: unknown
    } bits;

    WORD w[2];
    LWORD lw;
} U_L1_ENTRY;

typedef union
{
    struct
    {
        LWORD Offset2k : 26; // 2k offset within a map block
        LWORD BlkIdx : 4; //map block index
        LWORD sts : 2; // 00: normal, 01: pending VPC, 10: rsv, 11: unmap
    } bits;

    LWORD lw;

} U_L2L3_ENTRY;

typedef struct
{
    BYTE bPrev[C_L1_CACHE_CNT + C_EXT_L1_CACHE_CNT];
    BYTE bNext[C_L1_CACHE_CNT + C_EXT_L1_CACHE_CNT];
    WORD wSegNo[C_L1_CACHE_CNT + C_EXT_L1_CACHE_CNT];
    BYTE bSts[C_L1_CACHE_CNT + C_EXT_L1_CACHE_CNT];
    BYTE bFreeHead;
    BYTE bFreeTail;
    BYTE bCacheHead;
    BYTE bCacheTail;
    BYTE bFreeCnt;
    BYTE bCacheCnt;
    BYTE bL2PMode; //0: Normal, 1: Sequential, 2: Extend
    BYTE bExtL2PBuffSel;
    BYTE bDirtyCnt;
    BYTE bExtL2PForRW; // bit0 :read open ExtL2P, bit1: wirte open ExtL2P
} ST_L1_INFO;

typedef struct
{
    BYTE bPrev[C_L2_CACHE_CNT];
    BYTE bNext[C_L2_CACHE_CNT];
    BYTE bSegNo[C_L2_CACHE_CNT];
    BYTE bSts[C_L2_CACHE_CNT];
    BYTE bFreeHead;
    BYTE bFreeTail;
    BYTE bCacheHead;
    BYTE bCacheTail;
    BYTE bFreeCnt;
    BYTE bCacheCnt;
    BYTE bDirtyCnt;
} ST_L2_INFO;

typedef struct
{
    WORD wSegNo[C_P2L_CACHE_CNT];
    BYTE bSts[C_P2L_CACHE_CNT];
} ST_P2L_INFO;

/*Address translation related struct*/
typedef struct
{
    BYTE bCENum;
    BYTE bCENumShift;

    BYTE bEntrySize;
    BYTE bEntrySizeShift;

    BYTE bEntryNum;
    BYTE bEntryNumShift;

    BYTE bPageSize;
    BYTE bPageSizeShift;

    BYTE bPlaneNum;
    BYTE bPlaneNumShift;

    WORD wSlcPageNum;
    BYTE bSlcPageNumShift;

    WORD wTlcPageNum;
    BYTE bTlcPageNumShift;

    WORD w1KBCodewordLen;

    WORD wTotalEntryPerSlcBlock;
    WORD wTotalEntryPerTlcBlock;

    BYTE bTotalP2lCntPerBlock;

    BYTE bAddrCalFactor;
} ST_DEV_CONFIG;

typedef struct
{
    BYTE b4kInfoWidth;
    BYTE b4kInfoMask;

    BYTE bPlnInfoWidth;
    BYTE bPlnInfoMask;
    BYTE bPlnInfoShift;

    BYTE bBankInfoWidth;
    BYTE bBankInfoMask;
    BYTE bBankInfoShift;

    WORD wPageInfoMask;
    BYTE bPageInfoWidth;
    BYTE bPageInfoShift;
} ST_ADDRESS_CONFIG;

typedef struct
{
    WORD wBlkNo[C_MAX_MAP_BLK_NUM];
    WORD wVpc[C_MAX_MAP_BLK_NUM];
    BYTE bOrder[C_MAX_MAP_BLK_NUM];
    WORD wPagePtr;
    BYTE bOpenIdx;
    BYTE bTotalUsedCnt;
    BYTE bChgBlk;
    BYTE bGcStart;
    BYTE bGcSrcIdx;
    BYTE bGcScanL2SegNo;
    WORD wGcScanL2Offset;
    WORD wGcVpc;
} ST_MAP_BLK_INFO;

typedef struct
{
    WORD wBlkNo;
    BYTE bIdx;
    WORD wPagePtr;
    LWORD lwOrder;
} ST_IDX_BLK_INFO;

#define C_GC_FLAG_NONE      0
#define C_GC_FLAG_WROTE     1
#define C_GC_FLAG_DID_GC    2

typedef struct
{
    WORD wBlkNo;
    WORD wPtr4k;

    WORD wL2pUpdatePtr4k;
    LWORD lwHeaderSel;
    LWORD lwTempDataPtr;
    BYTE bBlkSn;
    BYTE bGcDidFlag;
} ST_HOST_OPEN_BLK_INFO, *ST_HOST_OPEN_BLK_INFO_PTR;

typedef struct
{
    WORD wBlkNo;
    WORD wPtr4k;
    LWORD lwL4k[C_TOTAL_4K_PER_MPAGE];
    BYTE bTmpCachedPtr;
} ST_TMP_BLK_INFO;

typedef struct
{
    WORD wBlkNo[C_MAX_SLC_FREEQ_NUM];
    BYTE bOrder[C_MAX_SLC_FREEQ_NUM];
    BYTE bOpenIdx;
    BYTE bTotalUsedCnt;
} ST_SLC_CACHE_INFO;


/*Gc module related*/
#define C_MAX_GC_SCANNED_BLK_NO             16
#define C_HOST_WRITE_4KB_CNT_TO_GC          16//32

#define C_TOTAL_4K_PER_GC_PROGRAM           24 // TLC: 3 * MPAGE

#define C_GC_MODE_NONE                      (0)
#define C_GC_MODE_SLC_CACHE                 (1)
#define C_GC_MODE_TLC                       (2)
#define C_GC_MODE_WL                        (3)
#define C_GC_MODE_RC                        (4)
#define C_GC_MODE_PSR                       (5)

#define C_SLC_URGENT                        cB_BIT0
#define C_TLC_URGENT                        cB_BIT1

#define C_GC_STATE_INIT                     (0)
#define C_GC_STATE_INIT_BRIDGE              (1)
#define C_GC_STATE_COPY_RW                  (2)
#define C_GC_STATE_INTERNAL_COPYBACK        (3)
#define C_GC_STATE_READ_VERIFY              (4)
#define C_GC_STATE_UPDATE_L2P               (5)
#define C_GC_STATE_RELEASE_BLK              (6)
#define C_GC_STATE_ERROR_STOP               (7)

//definition for gc status
#define C_GC_FLUSH_P2L_STEP1                cB_BIT0
#define C_GC_FLUSH_P2L_STEP2                cB_BIT1
#define C_GC_FLUSH_P2L_STEP3                cB_BIT2
#define C_GC_FORCE_STOP                     cB_BIT3
#define C_GC_READ_VERIFY_FAIL               cB_BIT4
#define C_GC_FORCE_EXIT                     cB_BIT5

#define C_TLCMODE                           (0)
#define C_SLCMODE                           (1)

#define C_SRCH_MAX                          (0)
#define C_SRCH_MIN                          (1)

// threshold part should be modified for YMTC X1-9050
#define C_GC_THRESHOLD_TLC                  (8)
#define C_GC_THRESHOLD_SLC_CACHE            (12)
#define C_GC_THRESHOLD_PSR                  (4)

#define C_GC_THRESHOLD_TLC_URGENT           (3)
#define C_GC_THRESHOLD_SLC_CACHE_URGENT     (6)
#define C_GC_THRESHOLD_PSR_URGENT           (2)

#define C_GC_THRESHOLD_TLC_IDLE             (25)
#define C_GC_THRESHOLD_SLC_CACHE_IDLE       (20)

#define C_WL_SRC_BIT                        (0x8000)
#define C_TLC_FREE_BIT                       (0x4000)

#define C_GC_SRC_BIT                        (0x80000000)
#define C_GC_DES_BIT                        (0x80000000)

#define M_SET_WL_SRC_BLK(x)                 (guwEcTable[x] |= C_WL_SRC_BIT)
#define M_CLR_WL_SRC_BLK(x)                 (guwEcTable[x] &= ~C_WL_SRC_BIT)

#define M_SET_TLC_FREE_BLK(x)                 (guwEcTable[x] |= C_TLC_FREE_BIT)
#define M_CLR_TLC_FREE_BLK(x)                 (guwEcTable[x] &= ~C_TLC_FREE_BIT)

#define M_SET_GC_SRC_BLK(x)                 (gulVpcTable[x] |= C_GC_SRC_BIT)
#define M_SET_GC_DES_BLK(x)                 (gulVpcTable[x] |= C_GC_DES_BIT) //set the same bit as src bit for not being selected by SE when finding gc src blk

#define M_CLR_GC_SRC_BLK(x)                 (gulVpcTable[x] &= ~C_GC_SRC_BIT)
#define M_CLR_GC_DES_BLK(x)                 (gulVpcTable[x] &= ~C_GC_DES_BIT)

#define M_CHK_GC_SRC_BLK(x)                 ((gulVpcTable[x] & C_GC_SRC_BIT) ? TRUE: FALSE)
#define M_CHK_GC_DES_BLK(x)                 ((gulVpcTable[x] & C_GC_DES_BIT) ? TRUE: FALSE)

#define C_PEND_VPC_HANDLE                   (0x80000000)

typedef struct
{
    WORD wDesBlk;
    WORD wDesPtr4k;
    WORD wDesL2pUpdatePtr4k;    //within 1 gc des blk
    WORD wDesUpdateBoundary[C_MAX_GC_SCANNED_BLK_NO];    //indicate the range of data of des blk belongs to the corresponding src blk
    WORD wDesRVPtr4k;

    WORD wSrcBlk;
    WORD wSrcScanPtr4k;    //for TLC
    WORD wSrcBlkVpc;
    WORD wSrcBlkQ[C_MAX_GC_SCANNED_BLK_NO];

    WORD wBridgeBlk[3]; //Could be removed, Iras!!!
    WORD wBridgeBlkPtr4k; //Could be removed, Iras!!!
    BYTE bBridgeCurrIdx; //Could be removed, Iras!!!

    BYTE bSrcBlkQHead;
    BYTE bSrcBlkQTail;
    BYTE bScannedBlkCnt;    //indicate how many src blk has been scanned done

    BYTE bDesMode;  //SlcMode or TlcMode
    BYTE bSrcMode;

    WORD wGcCnt;
    WORD wGcScanPageCnt; //MPage based
    BYTE bDataCnt4k; //accumulated 4k data count for programming
    BYTE bGcBufPtr;
    BYTE bGc2PassState; //Could be removed, Iras!!!

    BYTE bGcUrgentSts;

    BYTE bGcMode;
    BYTE bGcState;
    BYTE bGcStatus;

    BYTE bGcValidBitMap;
    BYTE bTempScanLoopCnt;
    BYTE bPreparedDataCnt4k;
    BYTE bGcProg4kCntForMoveP2L;
    BYTE bGcProg4kCntPerLoop;

    LWORD lwRecL4k[C_TOTAL_4K_PER_GC_PROGRAM];
} ST_GC_INFO, *ST_GC_INFO_PTR ;

/* read related struct */
typedef enum //1byte
{
    E_READ_START = 0,
    E_CMD_PROC,
    E_RANDOM_HANDLE,
    E_LOAD_L2P,
    E_GET_DATA_SRC,
    E_READ_FLASH_DATA,
    E_SEND_HOST_DATA,
    E_READ_END,
    E_STATE_SIZE
} E_READ_STATE;

typedef enum
{
    CACHE_READ_NON = 0,
    CACHE_READ_INIT,
    CACHE_READ_START,
    CACHE_READ_DATA_OUT,
    CACHE_READ_END
} CACHE_READ_STATE;


typedef E_READ_STATE(*HostReadFunc)(void);

typedef struct addr_info //12byte
{
    WORD wBlkNo;
    WORD wPageNo;

    WORD wPartiNo;
    BYTE bBankNo;
    BYTE bPlaneNo;

    BYTE bMode;
    BYTE bRes[3];
} ST_ADDRESS_INFO;

typedef struct read_para //200byte
{
    ST_ADDRESS_INFO stBakAddrInfo[2];
    ST_ADDRESS_INFO stAddrInfo;
    U_L1_ENTRY      stL1Entry;

    LWORD ulLastReadSecCnt;     //record last read sector count
    LWORD ulReadCmdQStartLBA[MAX_READ_CMDQ];

    WORD  wMinXfrSecCnt;        //transfer sector count in binding page(ce cnt * plane per ce * sector per plane)
    WORD  wSectorCnt;           //total continuous sector count at l2p or temp

    WORD  wEarlySendCnt;        //when read cnt > auto buffer size, cut off length and early send to host
    BYTE  bRandomReadTimes;
    BYTE  bDataType;            //indicate seq or random read flow

    BYTE  bCacheReadState;
    BYTE  bRandomRead;          //set true or false to indicate single plane random read
    BYTE  bQFreeCnt;
    BYTE  bQLastIdx;

    BYTE  bEndCeNo;
    BYTE  bPlnCnt;
    BYTE  bCacheReadInfoIdx;
    BYTE  bL1CacheIdx;

    WORD  wTrigerNum;
    WORD  wNowLength;

    WORD  wReadCnt;
    BYTE  bOpSec;
    BYTE  bOp4K;

    LWORD ulCacheSts;

    BYTE  CacheReadDelayArg;
    BYTE  bInReadFlow;
    BYTE  bReadTimerStart;
    BYTE  bTlcCacheReadPages;

    BYTE  bFlashTrigPtr[MAX_READ_CMDQ];
    BYTE  bRsv[44];
} ST_READ_PARA, * ST_READ_PARA_PTR;

typedef struct retry_para
{
    WORD  wBlkNo;
    WORD  wPageNo;

    BYTE  bBankNo;
    BYTE  bPlaneNo;
    BYTE  bMode;
    BYTE  bPartiNo1K;

    BYTE  bEndParti1kNo;
    BYTE  bLDPCSoftMode;
    BYTE  bRetryType;
    BYTE  bFirstEccPos;

    BYTE  bBlockType;
    BYTE  bNeedSecCnt;
    BYTE  bStartSecPos;
    BYTE  bBufPos;

    BYTE  bLastSlcVthGroup;
    BYTE  bLastTlcVthGroup;
    BYTE  bMultiEn;
    BYTE  bSetBufSts;

    WORD  wTrigPtr;
    BYTE  bEccFail;
    BYTE  bUsing96kBuf;

    BYTE  bRestoreVt;
    BYTE  bTrigPtr; //26byte
    BYTE  bMetaRetryRes;
    BYTE  bIsEmptyPage;//28byte

    LWORD lwRes;//32byte

    LWORD lwSecErrSts[2];
    BYTE  bEccTable[C_TOTAL_1K_PER_HALF_MPAGE * 2];
} ST_RETRY_PARA;

typedef struct //8byte
{
    WORD  wBlockNo;  // super block
    WORD  wPagePtr;  // physical page order in a super block

    WORD  wDfxBlkTail;
    WORD  wDfxBlkHead;
    WORD  wDfxBlkCnt;
} ST_DFX_INFO, * ST_DFX_INFO_PTR;

typedef struct //12byte
{
    LWORD ulStartLba;
    LWORD ulReocrdLen;

    BYTE  bCmd;
    BYTE  bRsv[3];
} ST_HOST_BEHAVIOR_RECORD;


typedef enum
{
    E_DFX_READ = 0x10,
    E_DFX_WRITE = 0x20,
    E_DFX_GC = 0x30,
    E_DFX_MAP = 0x40,
    E_DFX_FCL = 0x50,
    E_DFX_RB = 0x60,
    E_DFX_BLKMGT = 0xA0,
    E_DFX_CMD = 0xC0,
    E_DFX_CMD_END = 0xC1,
    E_DFX_ERR = 0xF0
} E_DFX_TYPE;

typedef enum
{
    E_ALLOC_SLC = 0x10,
    E_ALLOC_TLC,
    E_RELEASE_BLK,
    E_FREE_CNT,
    //Read option  0X10 ~ 0X19
    E_READ_FLOW_START = 0x20,
    E_READ_FLOW_END,
    E_READ_UNC,
    E_READ_UNC_END,
    E_READ_UNC_ENTRY,
    E_READ_OUT_OFF_RANGE,

    //GC option    0X20 ~ 0X29
    E_GC_ALLOC_TLC = 0x30,
    E_GC_START,
    E_GC_ALLOC_GC_SRC,
    E_GC_CB_SRC,
    E_GC_IN,
    E_GC_OUT,
    E_GC_OP_1,
    E_GC_RELSDES,

    E_MAP_GC = 0x40,

    //ADD OPTION YOU NEED AND DEFINE OPTION NAME

} E_DFX_OP_CODE;

typedef enum
{
    //Read Error code  0X0000 ~ 0X0FFF
    //E_ERR_0000 = 0X0000,
    //E_ERR_0001,

    //Write Error code  0X1000 ~ 0X1FFF
    E_ERR_1000 = 0X1000,
    E_ERR_1001,

    //Gc Error code 0X2000 ~ 0X2FFF
    E_ERR_2000 = 0X2000,
    E_ERR_2001,

    //Blk management
    E_ERR_3005 = 0x3005,
    E_ERR_3006,

    //ADD ERROR CODE YOU NEED AND DEFINE ERROR CODE NAME
    //Read Error code  0X4000 ~ 0X4FFF
    E_ERR_4000 = 0X4000,
    E_ERR_4001, //out of range
    E_ERR_4002,

} E_DFX_ERR_CODE;

typedef struct
{
    BYTE bCE;
    BYTE bPlane;
    WORD wPage;
    WORD wBlk;
} ST_DFX_MISC;

typedef struct
{
    BYTE bType;
    BYTE bOpCode;

    union
    {
        //ADD MSG YOU NEED
        ST_DFX_MISC stMsic;
        BYTE b[DFX_MSG_SIZE - 2];
    } u;

} ST_DFX_MSG;

typedef union
{
    ST_DFX_MSG stMsg;

    BYTE  b[DFX_MSG_SIZE];
    WORD  w[(DFX_MSG_SIZE >> 1)];
    LWORD l[(DFX_MSG_SIZE >> 2)];
} U_DFX_MSG, * U_DFX_MSG_PTR;

typedef struct
{
    U_DFX_MSG uDfxMsg;
    WORD  wPagePtr;   // open page ptr
    WORD  wMsgCnt;    // total msg count

    BYTE  bPlaneIdx;  // plane index
    BYTE  bMsgFull;   // 1: msg full
    BYTE  bCENo;
    BYTE  bForceFlush;
} ST_DFX_RUN_TIME_INFO, * ST_DFX_RUN_TIME_INFO_PTR;

//typedef struct
//{
//    WORD wBlkPtr;
//    WORD wHeadPtr;
//
//    WORD wTailPtr;
//    WORD wDfxBlkCnt;
//
//    //WORD wRes[124];//256byte
//
//    WORD wBlock[MAX_DFX_BLOCK_CNT];//MAX (3328byte - 1K - 8 byte - 8byte) / 2 blocks
//}* P_DFX_BLOCK_ARR;


/*GI variables*/
typedef struct
{
    WORD wSlcFreeQueue[C_MAX_SLC_FREEQ_NUM];
    WORD wTlcFreeQueue[C_MAX_TLC_FREEQ_NUM];

    ST_DEV_CONFIG stDevConfig;
    ST_ADDRESS_CONFIG stSlcAddrConfig;
    ST_ADDRESS_CONFIG stTlcAddrConfig;
    ST_IDX_BLK_INFO stIdxBlkInfo;
    ST_BIB_INFO stBIBInfo;
    ST_MAP_BLK_INFO stMAPBlkInfo;

    ST_HOST_OPEN_BLK_INFO stHostOpnBlkInfo;
    ST_TMP_BLK_INFO stTempBlkInfo;
    ST_SLC_CACHE_INFO stSlcCacheInfo;
    ST_HOST_OPEN_BLK_INFO stPSROpenBlkInfo;

    ST_GC_INFO stGcInfo;
    ST_GC_INFO stPSRGcInfo;

    ST_READ_PARA stReadPara;
    ST_DFX_INFO  stDfxInfo;

    WORD wBListLecFreeHead;
    WORD wBListLecFreeTail;
    WORD wBListLecFreeCnt;
    WORD wBListHecFreeHead;
    WORD wBListHecFreeTail;
    WORD wBListHecFreeCnt;

    WORD wBListSLCLecFreeHead;
    WORD wBListSLCLecFreeTail;
    WORD wBListSLCLecFreeCnt;
    WORD wBListSLCHecFreeHead;
    WORD wBListSLCHecFreeTail;
    WORD wBListSLCHecFreeCnt;

    WORD wFirstTlcBlkNo;
    WORD wFirstSlcBlkNo;
    WORD wFirstPSRBlkNo;

    BYTE bSlcFreeCnt;
    BYTE bSlcFreeHead;
    BYTE bSlcFreeTail;
    BYTE bTlcFreeCnt;
    BYTE bTlcFreeHead;
    BYTE bTlcFreeTail;
    BYTE bBLUpdate;
    BYTE bForceGcMapBlkCnt;

    WORD wWLQueue[C_MAX_WL_Q_CNT];
    BYTE bWLQueueCnt;
    BYTE bCIDInfo[16];
    BYTE bCSDInfo[16];
    BYTE bMIDInfo[8];
    BYTE bNewLockSts;
    BYTE bNewPWLen;
    BYTE bNewPWData[16];
    BYTE bMaxPWDataLen;
    LWORD lwProtectedArea;
    LWORD lwMKBSize;
    LWORD lwL3Table[128];
    WORD wPSRFreeQueue[C_MAX_PSR_FREEQ_NUM];
    BYTE bPSRFreeCnt;
    BYTE bPSRFreeHead;
    BYTE bPSRFreeTail;
    LWORD lwGIOrder;
    LWORD lwRebuildUncCnt;
    LWORD lwRebuildRetryUncCnt;
    LWORD lwPORCnt;
	WORD wGlobalEraseCnt;
    BYTE bRsvByte[0xb3a];
} ST_GIVARS;
/*Reserved for ext L2P*/
typedef struct
{
    BYTE bTestByte[32 * 1024];
} ST_EXT_L2P_TABLE;

/*Reserved for DFX log*/
typedef struct
{
    BYTE bTestByte[3328];
} ST_DFX_LOG;

/*Reserved for     cache buffer*/
typedef struct
{
    BYTE bTestByte[4 * 1024];
} ST_CACHE_BUFFER;

/*Reserved for     RBT*/
typedef struct
{
    BYTE bTestByte[3 * 1024];
} ST_RBT_TABLE;

/*Reserved for     RBT*/
typedef struct
{
    BYTE bTestByte[1024];
} ST_REG_BACKUP_TABLE;


/*Reserved for BOOT info*/
typedef struct
{
    BYTE bTestByte[3 * 1024];
} ST_BOOT_INFO;

/*Reserved for ROM global variables*/
typedef struct
{
    BYTE bTestByte[664];
} ST_ROM_VARS;

/*Reserved for ROM variables*/
typedef struct
{
    BYTE bTestByte[1024];
} ST_ROM_VARS1;

/*FCL address information carrier*/
#pragma pack(1)
typedef union
{
    struct
    {
        unsigned short column;
        unsigned int row : 22;
        unsigned char counter;
    } x;

    /* YMTC X1-9050 TLC NAND flash */
    struct
    {
        unsigned short rsv0 : 9;
        unsigned short part : 6;
        unsigned short rsv15 : 1;

        unsigned int page : 11;     // PA[10:0]
        unsigned int plane : 1;     // BA[0]
        unsigned int block : 10;    // BA[10:1]
        unsigned int rsv22 : 1;
        unsigned int lun : 3;       // LA[2:0]

        unsigned int rsv26 : 2;
        unsigned int ce : 2;
        unsigned int enh : 1;
        unsigned int rbt : 1;
    } f;

    struct
    {
        unsigned short column;
        unsigned int page : 11;     // PA[10:0]
        unsigned int block : 11;    // BA[10:0]
        unsigned int rsv : 1;
        unsigned int lun : 3;

        unsigned int rsv27 : 1;
        unsigned int ce : 2;
        unsigned int enh : 1;
        unsigned int rbt : 1;
    } p;

    unsigned int ul[2];
    unsigned short uw[4];
    unsigned char ub[8];
} U_AddrRegs;
#pragma pack()

typedef struct
{
    LWORD lwMemAddr;
    WORD wBlock;
    WORD wPage;
    BYTE bPartNo;
    BYTE bLen512B;
    BYTE bMode;
#define DBG_VCMD_RETRY_EN       0x01
    BYTE bMisc;
} ST_DBG_VCMD, ST_DBG_VCMD_PTR;


/*Power on reset variables*/
typedef struct
{
    //BYTE bTestByte[8 * 1024];
    ST_L1_INFO stL1Info;
    ST_L2_INFO stL2Info;
    ST_P2L_INFO stP2lInfo;
    BYTE bRebuildSts;
    BYTE bFuncOutput;
    WORD wFuncOutput;
    WORD wFuncOutput1;
    ST_WRITE_CACHE_INFO stWrCacheInfo[2]; // PING-PONG buffer, first half and second half
    ST_WRITE_BUF_CONSOLE stBufConsole;
    ST_IDX_BLK_INFO stIdxBlkInfoBackup;

    ST_RETRY_PARA stRetryPara;
    ST_DFX_RUN_TIME_INFO stDfxRunTimeInfo;
    ST_HOST_BEHAVIOR_RECORD stLbaRecord;
    WORD wRblBlkNo;
    WORD wRblPageNo;
    WORD wRblPtr4k;
    BYTE bRblPlaneNo;
    BYTE bRblCeNo;
    BYTE bRblMode;
    LWORD lwEraseMisalignAddr;
    BYTE bEraseMisalignCnt;
    BYTE bVCMDState;
    BYTE bVCMDExc;
    ST_DBG_VCMD stVcmdInfo;
    BYTE bBlkType;
#ifdef WR_FLOW_VER2
    LWORD lwWrBackupPtr;
    BYTE bHostSideChkIdx;
#endif
    BYTE bRebuildSts2;
    BYTE bInputArg[8];
    WORD wInputArg[4];
    LWORD lwInputArg[3];

    BYTE bRebuildUpdateGI;
} ST_GLVARS_R;

/*Standby data cache*/
typedef struct
{
    BYTE bTestByte[7424];
} ST_STBY_CACHE;

typedef struct
{
    LWORD ulCache[C_L1_CACHE_CNT][C_L1_ENTRY_CNT];
} ST_L1_TABLE;

typedef struct
{
    LWORD ulCache[C_EXT_L1_CACHE_CNT][C_L1_ENTRY_CNT];
} ST_EXT_L1_TABLE;

typedef struct
{
    LWORD ulCache[C_L2_CACHE_CNT][C_L2_ENTRY_CNT];
} ST_L2_TABLE;

typedef struct
{
    LWORD ulCache[C_OPEN_P2L_CACHE_CNT][C_P2L_ENTRY_CNT];
} ST_P2L_TABLE;

typedef struct
{
    LWORD ulCache[C_P2L_ENTRY_CNT];
} ST_GC_SRC_P2L_TABLE;

typedef struct
{
    BYTE bRsv[7448];
} ST_WRITE_LOG;

#define BOOT_MARK       0x5442787837324d53
typedef struct
{
    //  NAND COMMAND,
    BYTE    XB_ReadFirstCmd;                //0xC000_1800 //1 0x00
    BYTE    XB_ReadLastCmd;                 //0xC000_1801 //1 0x30
    BYTE    XB_MultiReadFirstCmd;           //0xC000_1802 //1 0x60
    BYTE    XB_MultiReadLastCmd;            //0xC000_1803 //1 0x30
    BYTE    XB_MultiCacheReadLastCmd;       //0xC000_1804 //1 0x33
    BYTE    XB_CacheReadCmd;                //0xC000_1805 //1 0x31
    BYTE    XB_CacheReadEndCmd;             //0xC000_1806 //1 0x3f
    BYTE    XB_ProgramFirstCmd;             //0xC000_1807 //1 0x80
    BYTE    XB_DummyProgramCmd;             //0xC000_1808 //1 0x11
    BYTE    XB_MultiProgramCmd;             //0xC000_1809 //1 0x81
    BYTE    XB_ProgramLastCmd;              //0xC000_180A //1 0x10
    BYTE    XB_CacheProgramCmd;             //0xC000_180B //1 0x15
    BYTE    XB_EraseFirstCmd;               //0xC000_180C //1 0x60
    BYTE    XB_EraseLastCmd;                //0xC000_180D //1 0xD0
    BYTE    XB_MultiEraseFirstCmd;          //0xC000_180E //1 0x60
    BYTE    XB_MultiEraseLastCmd;           //0xC000_180F //1 0xD0
    BYTE    XB_ReadChipStatusCmd[4];        //0xC000_1810 //1*4   0xf1,0xf2,0xf3,0xf4
    BYTE    XB_ReadStatusCmd;               //0xC000_1814 //1 0x70
    BYTE    XB_RandomInCmd;                 //0xC000_1815 //1 0x85
    BYTE    XB_ONFIResetCmd;                //0xC000_1816 //1 0xfc
    BYTE    XB_ReadStatusEnhCmd;            //0xC000_1817 //1 0x78, 0x73

    //Global Var,0xF002_1000    0x1000~0x10ff
    WORD    XW_InfoFBlock[2];               //0xC000_1818 //2*2
    WORD    XW_ISPFBlock[2];                //0xC000_181C //2*2
    BYTE    XB_InfoFCE[2];                  //0xC000_1820 //1*2
    BYTE    XB_ISPFCE[2];                   //0xC000_1822 //1*2
    BYTE    XB_BootFBlock[2];               //0xC000_1824 //2
    BYTE    XB_BootFCE[2];                  //0xC000_1826 //2
    BYTE    XB_FCE_Table[8];                //0xC000_1828 //8         //  8CE
    BYTE    XB_LUNsPerCE;                   //0xC000_1830 //1         //(1~4)
    BYTE    XB_PageSize;                    //0xC000_1831 //1         //??1k(1~16)
    WORD    XW_WBlockPerLUN;                //0xC000_1832 //2         //<16k  //記錄一個LUN 有多少SYS1WFBlock
    WORD    XW_FBlockPerLUN;                //0xC000_1834 //2         //<16k  //記錄一個LUN 有多少SYS1FBlock
    WORD    XW_PagePerBlock;                //0xC000_1836 //2         //預定最大1024
    BYTE    XB_ShiftForFBlock;              //0xC000_1838 //1         //LUN有多少可定址的Log-FBlock,2^n(0~14)
    BYTE    XB_ShiftForFPlane;              //0xC000_1839 //1         //Log-FBlock有多少FPlane,2^n(0~2)
    BYTE    XB_ShiftForFPage;               //0xC000_183A //1         //Log-FBlock有多少FPage,2^n(0~10)(192算256)
    BYTE    XB_ShiftForIntIntlv;            //0xC000_183B //1         //2^n(max3)
    BYTE    XB_ShiftForExtIntlv;            //0xC000_183C //1         //2^n(max3)
    BYTE    XB_FlashID;                     //0xC000_183D //1         //vendor
    BYTE    XB_FlashType;                   //0xC000_183E //1         //slc,mlc,tlc,qlc
    BYTE    XB_TotalCENum;                  //0xC000_183F //1         //Max 8 CE
    WORD    XW_Sys1SpareBlock[32];          //0xC000_1840 //2*32      //boot*2 info*2 isp*2 Link*4 qtab*8 wpro*1
    BYTE    XB_Rsv1[64];                    //0xC000_1880 //2*32
    BYTE    XB_Sys1SprBlockCE[32];          //0xC000_18C0 //1*32
    BYTE    XB_Rsv25[32];                   //0xC000_18E0 //1*32
    BYTE    XB_Sys1BlkNum;                  //0xC000_1900 //1         //
    BYTE    XB_Sys1Mode;                    //0xC000_1901 //1         //bit 0 withbootcode,bit1 withSeedtab,bit2 remp,bit3 rm,bit4 Sys1Strong, bit5 Sys1RM
    BYTE    XB_RTBBInSys1;                  //0xC000_1902 //1         //
    BYTE    XB_Sys2Mode;                    //0xC000_1903 //1         //
    BYTE    XB_EccBit;                      //0xC000_1904 //1         // 13~72
    BYTE    XB_EccMode;                     //0xC000_1905 //1         //bit0 Sel6/Sel10,bit 1 metaSeed,bit2 DataShaping,bit3 metaShaping
    BYTE    XB_Sys1BootMode;                //0xC000_1906 //1
    BYTE    XB_Sys2Mode1;                   //0xC000_1907 //1
    WORD    XW_TotalHBlock;                 //0xC000_1908 //2
    WORD    XW_TotalFBlock;                 //0xC000_190A //2
    WORD    XW_HBlockPerQMU;                //0xC000_190C //2
    WORD    XW_BootDataStartColAddr;        //0xC000_190E //2
    WORD    XW_RandSeed[2];                 //0xC000_1910 //2*2
    WORD    XW_FlashMetaStartColAddr;       //0xC000_1914 //2
    WORD    XW_FlashDataStartColAddr;       //0xC000_1916 //2
    WORD    XW_BootMetaStartColAddr;        //0xC000_1918 //2
    WORD    XW_BootRandSeed;                //0xC000_191A //2
    BYTE    XB_FlashOpt;                    //0xC000_191C //1         //bit0 multiread with 5ale
    BYTE    XB_FlashIgnore;                 //0xC000_191D //1         //bit0 ignore erasefail, bit1 ignore programfail, bit2
    BYTE    XB_EccDataThValue;              //0xC000_191E //1
    BYTE    XB_EccMetaThValue;              //0xC000_191F //1

#if 0 //for ISP customize
    BYTE    XB_T1BlkChkNum;                 //0xC000_1920 //1
    BYTE    XB_MinTLCSprBlk1;               //0xC000_1921 //1
    BYTE    XB_SlcWLDiffCnt;                //0xC000_1922 //1
    BYTE    XB_TlcWLDiffCnt;                //0xC000_1923 //1
    BYTE    XB_Rsv5;                        //0xC000_1924 //1
    BYTE    XB_Rsv6;                        //0xC000_1925 //1
    BYTE    XB_Rsv7;                        //0xC000_1926 //1
    BYTE    XB_Rsv8;                        //0xC000_1927 //1
    BYTE    XB_Rsv9;                        //0xC000_1928 //1
    BYTE    XB_Rsv10;                       //0xC000_1929 //1
    BYTE    XB_Rsv11;                       //0xC000_192A //1
    BYTE    XB_PollingDelayTimer;           //0xC000_192B //1
    WORD    XW_MvSeqPage;                   //0xC000_192C //2
    BYTE    XB_Rsv12;                       //0xC000_192E //1
    BYTE    XB_Rsv13;                       //0xC000_192F //1

    WORD    XW_Rsv14;                       //0xC000_1930 //2
    BYTE    XB_ReadRetryNum;                //0xC000_1932 //1
    BYTE    XB_EraseRetryNum;               //0xC000_1933 //1
    BYTE    XB_Rsv15;                       //0xC000_1934 //1
    BYTE    XB_Rsv16;                       //0xC000_1935 //1
    BYTE    XB_PerformanceOpt;              //0xC000_1936 //1
    BYTE    XB_Rsv17;                       //0xC000_1937 //1
    BYTE    XB_Rsv18;                       //0xC000_1938 //1
    BYTE    XB_Rsv19;                       //0xC000_1939 //1
    WORD    XW_SleepMaxCount;               //0xC000_193A //2
    BYTE    XB_Rsv20[36];                   //0xC000_193C //1
    //BYTE    XW_ForceMarkBad[16],            //0xC000_1950
#else
    BYTE    XB_MaxMapGrpNum;                //0xC000_1920 //1
    BYTE    XB_MaxChildNumInOneGrp;         //0xC000_1921 //1
    BYTE    XB_MaxFatNumInOneGrp;           //0xC000_1922 //1
    BYTE    XB_MaxQDataBlockNumInQGrp;      //0xC000_1923 //1
    BYTE    XB_BoundaryMapGrpNum;           //0xC000_1924 //1
    BYTE    XB_BoundaryQBlockNumInQGrp;     //0xC000_1925 //1
    BYTE    XW_BoundaryQValiddataNum;       //0xC000_1926 //2
    BYTE    XB_BoundarySpr2Num;             //0xC000_1928 //1
    BYTE    XB_BoundaryFatNumInOneGrp;      //0xC000_1929 //1
    BYTE    XB_WLSys2BlkEsCnt;              //0xC000_192A //1
    BYTE    XB_PollingDelayTimer;           //0xC000_192B //1
    WORD    XW_SlcAreaBlkNum;               //0xC000_192C //2
    BYTE    XW_MvRandPage;                  //0xC000_192E //2

    WORD    XW_DummyBusyDelayTimer;         //0xC000_1930 //2
    BYTE    XB_ReadRetryNum;                //0xC000_1932 //1
    BYTE    XB_EraseRetryNum;               //0xC000_1933 //1
    BYTE    XB_QuickFatValidNum;            //0xC000_1934 //1
    BYTE    XB_QuickFatChkBlkNum;           //0xC000_1935 //1
    BYTE    XB_PerformanceOpt;              //0xC000_1936 //1
    BYTE    XB_ReadRetryOpt;                //0xC000_1937 //1
    BYTE    XW_MarkBadMvNum;                //0xC000_1938 //1
    WORD    XW_SleepCount;                  //0xC000_193A //2
    BYTE    XB_Rsv20[34];                   //0xC000_193C //1
    //BYTE    XW_ForceMarkBad[16],            //0xC000_1950
    BYTE    XB_SlcAccessCmd;
    BYTE    XB_SlcAbortCmd;
#endif
    BYTE    XB_CIDInfo[16];                 //0xC000_1960 //16
    BYTE    XB_CSDInfo[16];                 //0xC000_1970 //16
    BYTE    XB_MIDInfo[8];                  //0xC000_1980 //8
    LWORD   XL_ProtectedArea;               //0xC000_1988 //4
    LWORD   XL_MKBSize;                     //0xC000_198C //4
#define FWOPT_SPEED_CLASS               (24)
#define FWOPT_SPEED_GRADE               (30)
    BYTE    XB_FWOption[84];                //0xC000_1990 //??
    BYTE    XB_Rsv21;                       //0xC000_19E4 //1
    BYTE    XB_Rsv22;                       //0xC000_19E5 //1

    //add for SearchBoot
    BYTE    XB_BootCodeLength[2];           //0xC000_19E6 //2, 1*2
    WORD    XW_BootCodeSeed[2];             //0xC000_19E8 //4, 2*2
    WORD    XW_BootCodeColAddr[2];          //0xC000_19EC //4, 2*2
    LWORD   XL_BootCodeRowAddr[2];          //0xC000_19F0 //8, 4*2

    //Mark Boot Table Serial
    //BYTE    XB_Mark[8];                     //0xC000_19F8 //8, 1*8
    LLWORD  XLL_Mark;
} ST_BOOT_TABLE, * PST_BOOT_TABLE;

typedef struct
{
    WORD XW_ErrorCode;
    BYTE XB_SpareCnt;
    BYTE XB_CardGrade;
} ST_PREF_INFO, * PST_PREF_INFO;

typedef union
{
    ST_PREF_INFO pref;

    BYTE ub[cStrongPageSize * 2];
    WORD uw[cStrongPageSize];
    LWORD ul[cStrongPageSize / 2];
} U_EXT_INFO, * PU_EXT_INFO;

typedef union
{
    tBootInfo boot_info;

    struct
    {
        WORD RSTable[cSeedTableSize];       //0xC000_1000 //2*1024
        ST_BOOT_TABLE boot_tab;             //0xC000_1800 //512
        U_EXT_INFO ext_info;                //0xC000_1A00 //512
    } st;

    BYTE ub[3 * 1024];
} U_BOOT_INFO, * PU_BOOT_INFO;


/*Vcmd command*/
#define VCMD_EN
#define C_ICCM_ADDR 0x10000
#define C_DCCM_ADDR 0xC0000000
#define C_PROGRAM_BUFA_ADDR 0xF0000000
#define C_PROGRAM_BUFB_ADDR 0xF0002000
#define C_PROGRAM_BUFB_SIZE 0x80
#define C_TSB_BUF_SEL 0
#define C_DCCM_SEL 1
#define C_ICCM_SEL 2





typedef enum
{
    E_DBG_STA_NONE,
    E_DBG_STA_INIT_1,
    E_DBG_STA_INIT_2,
    E_DBG_STA_EXEC
} DEBUG_VCMD_STA;

typedef union
{
    struct
    {
        LWORD bLen512B        : 8;
        LWORD wAddr           : 16;
        LWORD bType           : 5;
    } sram;

    struct
    {
        LWORD bPlane          : 1;
        LWORD wPageNo         : 11;
        LWORD wBlockNo        : 12;
    } blk;

    WORD wAddr;
    WORD wBlock;
    WORD wPage;
    BYTE bPartNo;
    BYTE bLen512B;
    BYTE bMode;

    WORD wL1SegNo;
    WORD wP1SegNo;

    BYTE b[4];
    WORD w[2];
    LWORD lw;
} U_DBG_VCMD_INFO, *U_DBG_VCMD_INFO_PTR;


typedef enum
{
    E_DOP_SET_DRAM_ADDR = 0x10,             // 0x10
    E_DOP_SET_ICCM_ADDR,                    // 0x11
    E_DOP_SET_TSB0_ADDR,                    // 0x12
    E_DOP_SET_TSB1_ADDR,                    // 0x13
    E_DOP_SET_PRGBUF_ADDR,                  // 0x14
    E_DOP_SET_SYSREG_ADDR,                  // 0x15
    E_DOP_SET_TSBREG_ADDR,                  // 0x16
    E_DOP_SET_SDREG_ADDR,                   // 0x17
    E_DOP_SET_NFCREG_ADDR,                  // 0x18
    E_DOP_SET_OTHERDMA_ADDR,                // 0x19
    E_DOP_SET_GI_ADDR,                      // 0x1A

    E_DOP_SET_BLOCK = 0x20,                 // 0x20
    E_DOP_SET_PAGE,                         // 0x21
    E_DOP_SET_PARTI,                        // 0x22
    E_DOP_SET_MODE,                         // 0x23
    E_DOP_SET_MISC,                         // 0x24

    E_DOP_SET_PPN_W0 = 0x30,                // 0x30
    E_DOP_SET_PPN_W1,                       // 0x31

    E_DOP_GET_DATA = 0x90,                  // 0x90

    E_DOP_GET_PAGE = 0xA0,                  // 0xA0
    E_DOP_GET_HEADER,                       // 0xA1
    E_DOP_PRG_PAGE,                         // 0xA2

    E_DOP_GET_L2P = 0xB0,                   // 0xB0
    E_DOP_GET_P2L,                          // 0xB1
    E_DOP_GET_BL,                           // 0xB2
    E_DOP_GET_EC,                           // 0xB3
    E_DOP_GET_VPC,                          // 0xB4
    E_DOP_GET_L1_INFO,                      // 0xB5
    E_DOP_GET_L2_INFO,                      // 0xB6
    E_DOP_GET_EXT_CSD,                      // 0xB7
    E_DOP_GET_EC_MERGE,                     // 0xB8

    E_DOP_GET_DFX = 0xC0,                   // 0xC0
    E_DOP_DEBUG_CHK = 0xD0,                 // 0xD0
    E_DOP_DEBUG_END = 0xE0,                 // 0xE0
} DEBUG_OP_TYPE;

#endif //METORAGE_STRUCT_H
