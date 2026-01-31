#pragma once
#include <winioctl.h>

#define HIDDEN_SECTOR_DEFAULT           (64)

typedef struct
{
    VOLUME_DISK_EXTENTS vde;
    STORAGE_DEVICE_NUMBER sdn;
    GET_LENGTH_INFORMATION gli;
#define DLI_SIZE     (sizeof(DRIVE_LAYOUT_INFORMATION) + 3 * sizeof(PARTITION_INFORMATION))
    UCHAR dli[DLI_SIZE];

    UINT32 cluster_size;
    CHAR volumeLabel[11];
} ST_FORMAT_INFO, *PST_FORMAT_INFO;
