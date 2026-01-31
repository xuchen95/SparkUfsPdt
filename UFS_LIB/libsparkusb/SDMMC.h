#pragma once

/* MMC commands */                      /* response type */
#define MMC_GO_IDLE_STATE           0   /* R0 */
#define MMC_SEND_OP_COND            1   /* R3 */
#define MMC_ALL_SEND_CID            2   /* R2 */
#define MMC_SET_RELATIVE_ADDR       3   /* R1 */
#define MMC_SWITCH                  6   /* R1B */
#define MMC_SELECT_CARD             7   /* R1 */
#define MMC_SEND_EXT_CSD            8   /* R1 */
#define MMC_SEND_CSD                9   /* R2 */
#define MMC_STOP_TRANSMISSION       12  /* R1B */
#define MMC_SEND_STATUS             13  /* R1 */
#define MMC_SET_BLOCKLEN            16  /* R1 */
#define MMC_READ_BLOCK_SINGLE       17  /* R1 */
#define MMC_READ_BLOCK_MULTIPLE     18  /* R1 */
#define MMC_SEND_TUNING_BLOCK       19  /* R1 */
#define MMC_SEND_TUNING_BLOCK_HS200 21  /* R1 */
#define MMC_SET_BLOCK_COUNT         23  /* R1 */
#define MMC_WRITE_BLOCK_SINGLE      24  /* R1 */
#define MMC_WRITE_BLOCK_MULTIPLE    25  /* R1 */
#define MMC_LOCK_UNLOCK             42  /* R1 */
#define MMC_APP_CMD                 55  /* R1 */
#define MMC_VEN_CMD60               60
#define MMC_VEN_CMD61               61

/* SD commands */                       /* response type */
#define SD_SEND_RELATIVE_ADDR       3   /* R6 */
#define SD_SEND_SWITCH_FUNC         6   /* R1 */
#define SD_SEND_IF_COND             8   /* R7 */
#define SD_VOLTAGE_SWITCH           11  /* R1 */
#define SD_PROGRAM_CSD              27  /* R1 */
#define SD_WRITE_PROT               28  /* R1B */
#define SD_CLR_WRITE_PROT           29  /* R1B */
#define SD_SEND_WRITE_PROT          30  /* R1 */
#define SD_ERASE_WR_BLK_START       32  /* R1 */
#define SD_ERASE_WR_BLK_END         33  /* R1 */
#define SD_ERASE                    38  /* R1B */

/* SD application commands */           /* response type */
#define SD_APP_SET_BUS_WIDTH        6   /* R1 */
#define SD_APP_OP_COND              41  /* R3 */
#define SD_APP_SEND_SCR             51  /* R1 */

/*
  MMC status in R1, for native mode (SPI bits are different)
  Type
    e : error bit
    s : status bit
    r : detected and set for the actual command response
    x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
    a : according to the card state
    b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
    c : clear by read
 */

#define R1_OUT_OF_RANGE             (1 << 31)   /* er, c */
#define R1_ADDRESS_ERROR            (1 << 30)   /* erx, c */
#define R1_BLOCK_LEN_ERROR          (1 << 29)   /* er, c */
#define R1_ERASE_SEQ_ERROR          (1 << 28)   /* er, c */
#define R1_ERASE_PARAM              (1 << 27)   /* ex, c */
#define R1_WP_VIOLATION             (1 << 26)   /* erx, c */
#define R1_CARD_IS_LOCKED           (1 << 25)   /* sx, a */
#define R1_LOCK_UNLOCK_FAILED       (1 << 24)   /* erx, c */
#define R1_COM_CRC_ERROR            (1 << 23)   /* er, b */
#define R1_ILLEGAL_COMMAND          (1 << 22)   /* er, b */
#define R1_CARD_ECC_FAILED          (1 << 21)   /* ex, c */
#define R1_CC_ERROR                 (1 << 20)   /* erx, c */
#define R1_ERROR                    (1 << 19)   /* erx, c */
#define R1_UNDERRUN                 (1 << 18)   /* ex, c */
#define R1_OVERRUN                  (1 << 17)   /* ex, c */
#define R1_CID_CSD_OVERWRITE        (1 << 16)   /* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP            (1 << 15)   /* sx, c */
#define R1_CARD_ECC_DISABLED        (1 << 14)   /* sx, a */
#define R1_ERASE_RESET              (1 << 13)   /* sr, c */
#define R1_STATUS(x)                (x & 0xFFF9A000)
#define R1_CURRENT_STATE(x)         ((x & 0x00001E00) >> 9) /* sx, b (4 bits) */
#define R1_READY_FOR_DATA           (1 << 8)    /* sx, a */
#define R1_SWITCH_ERROR             (1 << 7)    /* sx, c */
#define R1_EXCEPTION_EVENT          (1 << 6)    /* sr, a */
#define R1_APP_CMD                  (1 << 5)    /* sr, c */

#define R1_STATE_IDLE               0
#define R1_STATE_READY              1
#define R1_STATE_IDENT              2
#define R1_STATE_STBY               3
#define R1_STATE_TRAN               4
#define R1_STATE_DATA               5
#define R1_STATE_RCV                6
#define R1_STATE_PRG                7
#define R1_STATE_DIS                8