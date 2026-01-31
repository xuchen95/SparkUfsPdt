#include "pch.h"
#include "Fat32Volume.h"
#include "FsStructs.h"

Fat32Volume::Fat32Volume(LPCSTR szDevicePath /*= nullptr*/) : VolumeDevice(szDevicePath)
{

}

int Fat32Volume::begin(LPVOID lpvoid)
{
    uint8_t tmp[512];
    int ret = ERROR_SUCCESS;

    m_pFmt = (PST_FORMAT_INFO)lpvoid;
    m_sectorsPerCluster = m_pFmt->cluster_size;

    do
    {
        if (ret = readSectors(0, tmp))
        {
            TRACE("error code: %08x\n", ret);
            break;
        }

        if (ret = lockUnLock())
        {
            TRACE("error code: %08x\n", ret);
            break;
        }
    } while (0);

    return ret;
}

int Fat32Volume::format(uint32_t nSectorCount /*= 0*/)
{
    if (nSectorCount)
    {
        m_sectorCount = nSectorCount;
    }
    else
    {
        VOLUME_DISK_EXTENTS& vde = m_pFmt->vde;

        m_sectorCount = (vde.Extents[0].ExtentLength.QuadPart >> 9);
    }

    if (m_sectorCount < 0x400000)
    {
        return ERROR_FILE_SYSTEM_LIMITATION;
    }

    //m_capacityMB = (m_sectorCount + SECTORS_PER_MB - 1) / SECTORS_PER_MB;
    if (m_sectorsPerCluster == 0)
    {
        m_sectorsPerCluster = 64;
    }

    if (nSectorCount)
    {
        return makeFat32(BU32);
    }
    else
    {
        return makeFat32();
    }
}

int Fat32Volume::writeMbr()
{
    ZeroMemory(&m_Mbr, sizeof(m_Mbr));

#if USE_LBA_TO_CHS
    lbaToMbrChs(mbr->part->beginCHS, m_capacityMB, m_relativeSectors);
    lbaToMbrChs(mbr->part->endCHS, m_capacityMB,
        m_relativeSectors + m_totalSectors - 1);
#else  // USE_LBA_TO_CHS
    m_Mbr.part->beginCHS[0] = 1;
    m_Mbr.part->beginCHS[1] = 1;
    m_Mbr.part->beginCHS[2] = 0;
    m_Mbr.part->endCHS[0] = 0XFE;
    m_Mbr.part->endCHS[1] = 0XFF;
    m_Mbr.part->endCHS[2] = 0XFF;
#endif  // USE_LBA_TO_CHS

    m_Mbr.part->type = m_partType;
    m_Mbr.part->relativeSectors = m_relativeSectors;
    m_Mbr.part->totalSectors = m_totalSectors;
    m_Mbr.signature = MBR_SIGNATURE;

    return writeSectors(0, m_Mbr.bootCode);
}

int Fat32Volume::makeFat32(uint32_t relativeSectors /*= 0*/)
{
    uint32_t nc;
    uint32_t r;
    int ret;

    if (relativeSectors)
    {
        m_relativeSectors = relativeSectors;
    }

    for (m_dataStart = 2 * BU32; ; m_dataStart += BU32)
    {
        nc = (m_sectorCount - m_dataStart) / m_sectorsPerCluster;
        m_fatSize = (nc + 2 + (BYTES_PER_SECTOR / 4) - 1) / (BYTES_PER_SECTOR / 4);
        r = m_relativeSectors + 9 + 2 * m_fatSize;
        if (m_dataStart >= r)
        {
            break;
        }
    }

    m_reservedSectorCount = m_dataStart - m_relativeSectors - 2 * m_fatSize;
    m_fatStart = m_relativeSectors + m_reservedSectorCount;
    m_totalSectors = nc * m_sectorsPerCluster + m_dataStart - m_relativeSectors;
    if ((m_relativeSectors + m_totalSectors) <= 16450560)
    {
        // FAT32 with CHS and LBA
        m_partType = 0X0B;
    }
    else
    {
        // FAT32 with only LBA
        m_partType = 0X0C;
    }

    if (relativeSectors && (ret = writeMbr()))
    {
        return ret;
    }

    initPbr();
    m_Pbr.bpb.bpb32.sectorsPerFat32 = m_fatSize;
    m_Pbr.bpb.bpb32.fat32RootCluster = 2;
    m_Pbr.bpb.bpb32.fat32FSInfoSector = 1;
    m_Pbr.bpb.bpb32.fat32BackBootSector = 6;
    m_Pbr.bpb.bpb32.physicalDriveNumber = 0X80;
    m_Pbr.bpb.bpb32.extSignature = EXTENDED_BOOT_SIGNATURE;

    time_t rawtime;
    time(&rawtime);
    m_Pbr.bpb.bpb32.volumeSerialNumber = (unsigned int)rawtime;

    unsigned char label[] = "NO NAME    ";
    for (size_t i = 0; i < sizeof(m_Pbr.bpb.bpb32.volumeLabel); i++)
    {
        m_Pbr.bpb.bpb32.volumeLabel[i] = label[i];
    }

    if (m_pFmt != nullptr)
    {
        memcpy((CHAR *)m_Pbr.bpb.bpb32.volumeLabel, m_pFmt->volumeLabel, sizeof(m_Pbr.bpb.bpb32.volumeLabel));
    }
    m_Pbr.bpb.bpb32.volumeType[0] = 'F';
    m_Pbr.bpb.bpb32.volumeType[1] = 'A';
    m_Pbr.bpb.bpb32.volumeType[2] = 'T';
    m_Pbr.bpb.bpb32.volumeType[3] = '3';
    m_Pbr.bpb.bpb32.volumeType[4] = '2';
    m_Pbr.bpb.bpb32.volumeType[5] = ' ';
    m_Pbr.bpb.bpb32.volumeType[6] = ' ';
    m_Pbr.bpb.bpb32.volumeType[7] = ' ';

    TRACE("Write PBR\n");
    do
    {
        if ((ret = writeSectors(m_relativeSectors, (uint8_t*)&m_Pbr)) ||
            (ret = writeSectors((m_relativeSectors + 6), (uint8_t*)&m_Pbr)) ||
            (ret = initFatDir(32, (2 * m_fatSize + m_sectorsPerCluster))))
        {
            break;
        }
    } while (0);

    return ret;
}

#define SEC_BUF_CNT                 (128)
int Fat32Volume::initFatDir(uint8_t fatType, uint32_t sectorCount)
{
    uint8_t* secBuf;
    size_t n;
    int ret = ERROR_SUCCESS;

    do
    {
        secBuf = (uint8_t*)GlobalAlloc(GPTR, SECTOR2BYTE(SEC_BUF_CNT));
        if (secBuf == nullptr)
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

#ifdef _NORMAL_FORMAT
        TRACE("Writing FAT\n");
        for (uint32_t i = 1; i < sectorCount; i += SEC_BUF_CNT)
        {
            if (ret = writeSectors(m_fatStart + i, secBuf, SEC_BUF_CNT))
            {
                break;
            }
        }
#endif // _NORMAL_FORMAT

        // Allocate reserved clusters and root for FAT32.
        secBuf[0] = 0XF8;
        n = (fatType == 16) ? 4 : 12;
        for (size_t i = 1; i < n; i++)
        {
            secBuf[i] = 0XFF;
        }

        if ((ret = writeSectors(m_fatStart, secBuf)) ||
            (ret = writeSectors(m_fatStart + m_fatSize, secBuf)))
        {
            break;
        }
    } while (0);

    GlobalFree(secBuf);

    return ret;
}

void Fat32Volume::initPbr()
{
    m_Pbr.jmpInstruction[0] = 0XEB;
    m_Pbr.jmpInstruction[1] = 0X76;
    m_Pbr.jmpInstruction[2] = 0X90;
    for (uint8_t i = 0; i < sizeof(m_Pbr.oemName); i++)
    {
        m_Pbr.oemName[i] = ' ';
    }
    m_Pbr.bpb.bpb16.bytesPerSector = BYTES_PER_SECTOR;
    m_Pbr.bpb.bpb16.sectorsPerCluster = m_sectorsPerCluster;
    m_Pbr.bpb.bpb16.reservedSectorCount = m_reservedSectorCount;
    m_Pbr.bpb.bpb16.fatCount = 2;
    // skip rootDirEntryCount
    // skip totalSectors16
    m_Pbr.bpb.bpb16.mediaType = 0XF8;
    // skip sectorsPerFat16
    // skip sectorsPerTrack
    // skip headCount
    m_Pbr.bpb.bpb16.hidddenSectors = m_relativeSectors;
    m_Pbr.bpb.bpb16.totalSectors32 = m_totalSectors;
    // skip rest of bpb
    m_Pbr.signature = PBR_SIGNATURE;
}
