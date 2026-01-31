#pragma once

#pragma pack(1)
typedef union
{
    struct
    {
        unsigned char   mid;
        unsigned char   oid[2];
        unsigned char   pnm[5];
        unsigned char   prv;
        unsigned char   psn[4];
        unsigned char   mdt[2];
        unsigned char   crc;
    } st;

    unsigned char ub[0x10];
} U_SD_CID, *PU_SD_CID;

typedef union
{
    struct
    {
        //byte 0
        unsigned char rsv0 : 6;
        unsigned char CSD_STRUCTURE : 2;
        //byte 1
        unsigned char TAAC : 8;
        //byte 2
        unsigned char NSAC : 8;
        //byte 3
        unsigned char TRAN_SPEED : 8;
        //byte 4
        unsigned char CCC4 : 8;
        //byte 5
        unsigned char READ_BL_LEN : 4;
        unsigned char CCC5 : 4;
        //byte 6
        unsigned char C_SIZE6 : 4;
        unsigned char DSR_IMP : 1;
        unsigned char READ_BLK_MISALIGN : 1;
        unsigned char WRITE_BLK_MISALIGN : 1;
        unsigned char READ_BL_PARTIAL : 1;
        //byte 7.8.9
        unsigned char C_SIZE7 : 8;
        unsigned char C_SIZE8 : 8;
        unsigned char C_SIZE9 : 8;
        //byte 10
        unsigned char SECTOR_SIZE10 : 6;
        unsigned char ERASE_BL_EN : 1;
        unsigned char rsv10 : 1;
        //byte 11
        unsigned char WP_GRP_SIZE : 7;
        unsigned char SECTOR_SIZE11 : 1;
        //byte 12
        unsigned char WRITE_BL_LEN12 : 2;
        unsigned char R2W_FACTOR : 3;
        unsigned char rsv12 : 2;
        unsigned char WP_GRP_ENABLE : 1;
        //byte 13
        unsigned char rsv13 : 5;
        unsigned char WRITE_BL_PARTIAL : 1;
        unsigned char WRITE_BL_LEN13 : 2;
        //byte 14
        unsigned char rsv14 : 2;
        unsigned char FILE_FORMAT : 2;
        unsigned char TMP_WP : 1;
        unsigned char PERM_WP : 1;
        unsigned char COPY : 1;
        unsigned char FILE_FORMAT_GRP : 1;

        unsigned char rsv15 : 1;
        unsigned char CRC : 7;
    } st;

    unsigned char ub[0x10];
} U_SD_CSD, *PU_SD_CSD;

struct sd_scr
{
    unsigned char       sda_vsn;
    unsigned char       sda_spec3;
    unsigned char       sda_spec4;
    unsigned char       sda_specx;
    unsigned char       bus_widths;
#define SD_SCR_BUS_WIDTH_1  (1<<0)
#define SD_SCR_BUS_WIDTH_4  (1<<2)
    unsigned char       cmds;
#define SD_SCR_CMD20_SUPPORT   (1<<0)
#define SD_SCR_CMD23_SUPPORT   (1<<1)
};

struct sd_ssr
{
    unsigned int        au;         /* In sectors */
    unsigned int        erase_timeout;      /* In milliseconds */
    unsigned int        erase_offset;       /* In milliseconds */
};

struct sd_switch_caps
{
    unsigned int        hs_max_dtr;
    unsigned int        uhs_max_dtr;
#define HIGH_SPEED_MAX_DTR  50000000
#define UHS_SDR104_MAX_DTR  208000000
#define UHS_SDR50_MAX_DTR   100000000
#define UHS_DDR50_MAX_DTR   50000000
#define UHS_SDR25_MAX_DTR   UHS_DDR50_MAX_DTR
#define UHS_SDR12_MAX_DTR   25000000
#define DEFAULT_SPEED_MAX_DTR   UHS_SDR12_MAX_DTR
    unsigned int        sd3_bus_mode;
#define UHS_SDR12_BUS_SPEED 0
#define HIGH_SPEED_BUS_SPEED    1
#define UHS_SDR25_BUS_SPEED 1
#define UHS_SDR50_BUS_SPEED 2
#define UHS_SDR104_BUS_SPEED    3
#define UHS_DDR50_BUS_SPEED 4

#define SD_MODE_HIGH_SPEED  (1 << HIGH_SPEED_BUS_SPEED)
#define SD_MODE_UHS_SDR12   (1 << UHS_SDR12_BUS_SPEED)
#define SD_MODE_UHS_SDR25   (1 << UHS_SDR25_BUS_SPEED)
#define SD_MODE_UHS_SDR50   (1 << UHS_SDR50_BUS_SPEED)
#define SD_MODE_UHS_SDR104  (1 << UHS_SDR104_BUS_SPEED)
#define SD_MODE_UHS_DDR50   (1 << UHS_DDR50_BUS_SPEED)
    unsigned int        sd3_drv_type;
#define SD_DRIVER_TYPE_B    0x01
#define SD_DRIVER_TYPE_A    0x02
#define SD_DRIVER_TYPE_C    0x04
#define SD_DRIVER_TYPE_D    0x08
    unsigned int        sd3_curr_limit;
#define SD_SET_CURRENT_LIMIT_200    0
#define SD_SET_CURRENT_LIMIT_400    1
#define SD_SET_CURRENT_LIMIT_600    2
#define SD_SET_CURRENT_LIMIT_800    3
#define SD_SET_CURRENT_NO_CHANGE    (-1)

#define SD_MAX_CURRENT_200  (1 << SD_SET_CURRENT_LIMIT_200)
#define SD_MAX_CURRENT_400  (1 << SD_SET_CURRENT_LIMIT_400)
#define SD_MAX_CURRENT_600  (1 << SD_SET_CURRENT_LIMIT_600)
#define SD_MAX_CURRENT_800  (1 << SD_SET_CURRENT_LIMIT_800)
};
#pragma pack()
