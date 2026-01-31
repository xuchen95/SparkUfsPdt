#pragma once
#include <stdint.h>

#define MBR_SIGNATURE               (0xAA55)
#define PBR_SIGNATURE               (0xAA55)
#define BU16                        (128)
#define BU32                        (8192)
#define BYTES_PER_SECTOR            (512)
#define BYTES_PER_SECTOR_SHIFT      (9)
#define SECTOR_MASK                 (BYTES_PER_SECTOR - 1)
#define SECTORS_PER_MB              (0X100000 / BYTES_PER_SECTOR)
#define EXTENDED_BOOT_SIGNATURE     (0X29)
#define VOLUME_LABEL_LEN            (11)
#define BITMAP_CLUSTER              (2)
#define UPCASE_CLUSTER              (3)
#define ROOT_CLUSTER                (4)
#define BOOT_BACKUP_OFFSET          (12)
#define MINIMUM_UPCASE_SKIP         (512)

#pragma pack(1)
typedef struct mbrPartition
{
    uint8_t boot;
    uint8_t beginCHS[3];
    uint8_t type;
    uint8_t endCHS[3];
    uint32_t relativeSectors;
    uint32_t totalSectors;
} MbrPart_t;

typedef struct
{
    uint8_t bootCode[446];
    MbrPart_t part[4];
    uint16_t signature;
} ST_MBR, * PST_MBR;

typedef struct biosParameterBlockFat16
{
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint8_t fatCount;
    uint8_t rootDirEntryCount[2];
    uint8_t totalSectors16[2];
    uint8_t mediaType;
    uint16_t sectorsPerFat16;
    uint16_t sectorsPerTrtack;
    uint8_t headCount[2];
    uint32_t hidddenSectors;
    uint32_t totalSectors32;

    uint8_t physicalDriveNumber;
    uint8_t extReserved;
    uint8_t extSignature;
    uint32_t volumeSerialNumber;
    uint8_t volumeLabel[VOLUME_LABEL_LEN];
    uint8_t volumeType[8];
} BpbFat16_t;

typedef struct biosParameterBlockFat32
{
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint8_t fatCount;
    uint8_t rootDirEntryCount[2];
    uint8_t totalSectors16[2];
    uint8_t mediaType;
    uint16_t sectorsPerFat16;
    uint16_t sectorsPerTrtack;
    uint8_t headCount[2];
    uint32_t hidddenSectors;
    uint32_t totalSectors32;

    uint32_t sectorsPerFat32;
    uint16_t fat32Flags;
    uint16_t fat32Version;
    uint32_t fat32RootCluster;
    uint16_t fat32FSInfoSector;
    uint16_t fat32BackBootSector;
    uint8_t fat32Reserved[12];

    uint8_t physicalDriveNumber;
    uint8_t extReserved;
    uint8_t extSignature;
    uint32_t volumeSerialNumber;
    uint8_t volumeLabel[VOLUME_LABEL_LEN];
    uint8_t volumeType[8];
} BpbFat32_t;

typedef struct partitionBootSectorFat
{
    uint8_t jmpInstruction[3];
    char oemName[8];
    union
    {
        uint8_t bpb[109];
        BpbFat16_t bpb16;
        BpbFat32_t bpb32;
    } bpb;
    uint8_t bootCode[390];
    uint16_t signature;
} ST_PBR_FAT, *PST_PBR_FAT;

typedef struct biosParameterBlockExFat
{
    uint8_t mustBeZero[53];
    uint64_t partitionOffset;
    uint64_t volumeLength;
    uint32_t fatOffset;
    uint32_t fatLength;
    uint32_t clusterHeapOffset;
    uint32_t clusterCount;
    uint32_t rootDirectoryCluster;
    uint32_t volumeSerialNumber;
    uint16_t fileSystemRevision;
    uint16_t volumeFlags;
    uint8_t bytesPerSectorShift;
    uint8_t sectorsPerClusterShift;
    uint8_t numberOfFats;
    uint8_t driveSelect;
    uint8_t percentInUse;
    uint8_t reserved[7];
} BpbExFat_t;

typedef struct ExFatBootSector
{
    uint8_t  jmpInstruction[3];
    char     oemName[8];
    BpbExFat_t  bpb;
    uint8_t  bootCode[390];
    uint16_t  signature;
} ST_PBR_EXFAT, *PST_PBR_EXFAT;

typedef union
{
    ST_PBR_EXFAT st;

    uint32_t ul[(BYTES_PER_SECTOR / 4)];
    uint16_t uw[(BYTES_PER_SECTOR / 2)];
    uint8_t ub[BYTES_PER_SECTOR];
} U_PBR_EXFAT, *PU_PBR_EXFAT;

typedef struct
{
    uint8_t  type;
    uint8_t  mustBeZero;
    uint8_t  unicode[30];
} DirName_t;

const uint8_t EXFAT_TYPE_LABEL = 0X83;
typedef struct
{
    uint8_t  type;
    uint8_t  labelLength;
    uint8_t  unicode[22];
    uint8_t  reserved[8];
} DirLabel_t;

const uint8_t EXFAT_TYPE_BITMAP = 0X81;
typedef struct
{
    uint8_t  type;
    uint8_t  flags;
    uint8_t  reserved[18];
    uint32_t  firstCluster;
    uint64_t  size;
} DirBitmap_t;

const uint8_t EXFAT_TYPE_UPCASE = 0X82;
typedef struct
{
    uint8_t  type;
    uint8_t  reserved1[3];
    uint32_t checksum;
    uint8_t  reserved2[12];
    uint32_t firstCluster;
    uint64_t size;
} DirUpcase_t;
#pragma pack()