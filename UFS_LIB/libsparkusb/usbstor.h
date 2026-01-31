#pragma once
#define _NTSCSI_USER_MODE_
#if TOOLSET_VER > 141
#include <scsi.h>
#else
#include "scsi.h"
#endif // TOOLSET_VER

#pragma pack(1)
/* USB Mass Storage Command Block Wrapper */
typedef struct _CBW
{
#define USB_CBW_SIGNATURE               0x43425355
    unsigned long dwCBWSignature;       // dwCBWSignature: the constant 0x55 0x53 0x42 0x43(LSB)
    unsigned long dwCBWtag;             // dwCBWtag:
    unsigned long dwCBWXferLength;      // dwCBWXferLength:number of bytes to transfer
    unsigned char bCBWFlags;            // bmCBWFlags:
                                        //   Bit 7: direction - the device shall ignore this bit if the
                                        //   dCBWDataTransferLength field is zero, otherwise:
                                        //   0 = Data-Out from host to the device,
                                        //   1 = Data-In from the device to the host.
                                        //   Bit 6: obsolete. The host shall set this bit to zero.
                                        //   Bits 5..0: reserved - the host shall set these bits to zero.
    unsigned char bCBWlun;              // bmCBWlun:
    unsigned char bCBWCBLength;         // bCBWLength: 0x01..0x10
    CDB cdb;                            // CBWCB: the command descriptor block
} CBW, *PCBW;

/* USB Mass Storage Command Status Wrapper */
typedef struct _CSW
{
#define USB_CSW_SIGNATURE               0x53425355
    unsigned long dwCSWSignature;       // dwCSWSignature: the constant 0x53 0x42 0x53 0x55
    unsigned long dwCSWtag;             // dwCSWtag:
    unsigned long dwCSWResidue;         // dwCBWXferLength:  number of bytes not transferred
    unsigned char bCSWStatus;           // bCSWStatus:
                                        //  00h command Passed ("good status")
                                        //  01h command failed
                                        //  02h phase error
                                        //  03h to FFh reserved
} CSW, *PCSW;
#pragma pack()
