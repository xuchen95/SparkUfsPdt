/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spti.h

Abstract:

    These are the structures and defines that are used in the
    SPTI.C.

Author:

Revision History:

--*/
#include <ntddscsi.h>
#define _NTSCSI_USER_MODE_
#if TOOLSET_VER > 141
#include <scsi.h>
#else
#include "scsi.h"
#endif // TOOLSET_VER

#define SPT_CDB_LENGTH 32
#define SPT_SENSE_LENGTH 32
#define SPTWB_DATA_LENGTH 512

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, * PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, * PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;


typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS_EX {
#if TOOLSET_VER > 141
    SCSI_PASS_THROUGH_EX spt;
#else
    SCSI_PASS_THROUGH spt;
#endif // TOOLSET_VER
    UCHAR             ucCdbBuf[SPT_CDB_LENGTH - 1];       // cushion for spt.Cdb
    ULONG             Filler;      // realign buffers to double word boundary
    STOR_ADDR_BTL8    StorAddress;
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];     // buffer for DataIn or DataOut
} SCSI_PASS_THROUGH_WITH_BUFFERS_EX, * PSCSI_PASS_THROUGH_WITH_BUFFERS_EX;


typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER_EX {
#if TOOLSET_VER > 141
    SCSI_PASS_THROUGH_DIRECT_EX sptd;
#else
    SCSI_PASS_THROUGH_DIRECT sptd;
#endif // TOOLSET_VER
    UCHAR             ucCdbBuf[SPT_CDB_LENGTH - 1];       // cushion for sptd.Cdb
    ULONG             Filler;      // realign buffer to double word boundary
    STOR_ADDR_BTL8    StorAddress;
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER_EX, * PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER_EX;

//
// Command Descriptor Block constants.
//

#define CDB6GENERIC_LENGTH                   6
#define CDB10GENERIC_LENGTH                  10
#define CDB16GENERIC_LENGTH					 16

#define SETBITON                             1
#define SETBITOFF                            0
