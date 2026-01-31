#include "pch.h"
#include "ExFatVolume.h"
#include "upcase.h"

ExFatVolume::ExFatVolume(LPCSTR szDevicePath /*= nullptr*/) : Fat32Volume(szDevicePath)
{

}

int ExFatVolume::format(uint32_t nSectorCount /*= 0*/)
{
    if (nSectorCount)
    {
        m_sectorCount = nSectorCount;
    }
    else
    {
        VOLUME_DISK_EXTENTS& vde = m_pFmt->vde;

        m_relativeSectors = (vde.Extents[0].StartingOffset.QuadPart >> 9);
        m_sectorCount = (vde.Extents[0].ExtentLength.QuadPart >> 9);
    }
    //m_capacityMB = (m_sectorCount + SECTORS_PER_MB - 1) / SECTORS_PER_MB;
    // Min size is 512 MB
    if (m_sectorCount < 0X100000)
    {
        return ERROR_FILE_SYSTEM_LIMITATION;
    }

    if (m_sectorsPerCluster == 0)
    {
        m_sectorsPerCluster = 64;
    }

    return makeExFat();
}

#define TMP_BUF_SEC_CNT     256
int ExFatVolume::makeExFat()
{
    ST_PBR_EXFAT& pbr = m_uPbr.st;

    uint32_t bitmapSize;
    uint32_t checksum = 0;
    uint32_t clusterCount;
    uint32_t clusterHeapOffset;
    uint32_t fatLength;
    uint32_t fatOffset;
    uint32_t m;
    uint32_t ns;
    uint32_t sector;
    uint32_t sectorsPerCluster;
    uint32_t volumeLength;
    uint32_t sectorCount;
    uint8_t sectorsPerClusterShift;
    uint8_t vs;
    uint8_t* tmpBuf;
    int ret = ERROR_SUCCESS;

    tmpBuf = (uint8_t*)GlobalAlloc(GPTR, TMP_BUF_SEC_CNT * BYTES_PER_SECTOR);
    if (tmpBuf == nullptr)
    {
        return ERROR_ALLOTTED_SPACE_EXCEEDED;
    }

    // Determine partition layout.
    for (m = 1, vs = 0; m && m_sectorCount > m; m <<= 1, vs++) {}
    sectorsPerClusterShift = vs < 29 ? 8 : (vs - 11) / 2;
    sectorsPerCluster = 1UL << sectorsPerClusterShift;
    fatLength = 1UL << (vs < 27 ? 13 : (vs + 1) / 2);
    fatOffset = fatLength;
    clusterHeapOffset = 2 * fatLength;
    clusterCount = (m_sectorCount - 4 * fatLength) >> sectorsPerClusterShift;
    volumeLength = clusterHeapOffset + (clusterCount << sectorsPerClusterShift);

    ZeroMemory(&pbr, sizeof(pbr));
    pbr.jmpInstruction[0] = 0XEB;
    pbr.jmpInstruction[1] = 0X76;
    pbr.jmpInstruction[2] = 0X90;
    pbr.oemName[0] = 'E';
    pbr.oemName[1] = 'X';
    pbr.oemName[2] = 'F';
    pbr.oemName[3] = 'A';
    pbr.oemName[4] = 'T';
    pbr.oemName[5] = ' ';
    pbr.oemName[6] = ' ';
    pbr.oemName[7] = ' ';

    pbr.bpb.partitionOffset = m_relativeSectors;
    pbr.bpb.volumeLength = m_sectorCount;
    pbr.bpb.fatOffset = fatOffset;
    pbr.bpb.fatLength = fatLength;
    pbr.bpb.clusterHeapOffset = clusterHeapOffset;
    pbr.bpb.clusterCount = clusterCount;
    pbr.bpb.rootDirectoryCluster = ROOT_CLUSTER;
    pbr.bpb.volumeSerialNumber = m_sectorCount;
    pbr.bpb.fileSystemRevision = 0X100;
    pbr.bpb.volumeFlags = 0;
    pbr.bpb.bytesPerSectorShift = BYTES_PER_SECTOR_SHIFT;
    pbr.bpb.sectorsPerClusterShift = sectorsPerClusterShift;
    pbr.bpb.numberOfFats = 1;
    pbr.bpb.driveSelect = 0X80;
    pbr.bpb.percentInUse = 0;

    // 0: Boot Sector
    for (size_t i = 0; i < sizeof(pbr.bootCode); i++)
    {
        pbr.bootCode[i] = 0XF4;
    }
    pbr.signature = PBR_SIGNATURE;
    for (size_t i = 0; i < BYTES_PER_SECTOR; i++)
    {
        if (i == offsetof(ST_PBR_EXFAT, bpb.volumeFlags) ||
            i == (offsetof(ST_PBR_EXFAT, bpb.volumeFlags) + 1) ||
            i == offsetof(ST_PBR_EXFAT, bpb.percentInUse))
        {
            continue;
        }
        checksum = exFatChecksum(checksum, m_uPbr.ub[i]);
    }
    sector = 0;
    if (writeSectors(sector, m_uPbr.ub) ||
        writeSectors(sector + BOOT_BACKUP_OFFSET, m_uPbr.ub))
    {
        TRACE("write Boot Sectors fail!!!\n");
        ret = ERROR_WRITE_FAULT;
    }
    sector++;

    // 1~8: Extended Boot Sectors
    ZeroMemory(m_uPbr.ub, BYTES_PER_SECTOR);
    pbr.signature = PBR_SIGNATURE;
    for (int j = 0; j < 8; j++)
    {
        for (size_t i = 0; i < BYTES_PER_SECTOR; i++)
        {
            checksum = exFatChecksum(checksum, m_uPbr.ub[i]);
        }
        if (writeSectors(sector, m_uPbr.ub) ||
            writeSectors(sector + BOOT_BACKUP_OFFSET, m_uPbr.ub))
        {
            TRACE("write Extended Boot Sectors fail!!!\n");
            ret = ERROR_WRITE_FAULT;
        }
        sector++;
    }

    // 9: OEM Parameters and Reserved
    ZeroMemory(m_uPbr.ub, BYTES_PER_SECTOR);
    for (int j = 0; j < 2; j++)
    {
        for (size_t i = 0; i < BYTES_PER_SECTOR; i++)
        {
            checksum = exFatChecksum(checksum, m_uPbr.ub[i]);
        }
        if (writeSectors(sector, m_uPbr.ub) ||
            writeSectors(sector + BOOT_BACKUP_OFFSET, m_uPbr.ub))
        {
            TRACE("write OEM Parameter Sector fail!!!\n");
            ret = ERROR_WRITE_FAULT;
        }
        sector++;
    }

    // 11: Boot Checksum
    for (size_t i = 0; i < (BYTES_PER_SECTOR / 4); i++)
    {
        m_uPbr.ul[i] = checksum;
    }
    if (writeSectors(sector, m_uPbr.ub) ||
        writeSectors(sector + BOOT_BACKUP_OFFSET, m_uPbr.ub))
    {
        TRACE("write Boot CheckSum Sector fail!!!\n");
        ret = ERROR_WRITE_FAULT;
    }

    // 24 ~ (FatOffset - 24): FAT Alignment	Boot Sectors contain FatOffset.
    sector = fatOffset;
    ns = ((clusterCount + 2) * 4 + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR;

    ZeroMemory(m_uPbr.ub, BYTES_PER_SECTOR);
    // Allocate two reserved clusters, bitmap, upcase, and root clusters.
    m_uPbr.ub[0] = 0XF8;
    for (size_t i = 1; i < 20; i++)
    {
        m_uPbr.ub[i] = 0XFF;
    }

    if (writeSectors(sector, m_uPbr.ub))
    {
        TRACE("Initialize FAT fail!!!\n");
        ret = ERROR_WRITE_FAULT;
    }

#ifdef _NORMAL_FORMAT
    sectorCount = 1;
    while (sectorCount < ns)
    {
        if ((ns - sectorCount) >= TMP_BUF_SEC_CNT)
        {
            if (writeSectors((sector + sectorCount), tmpBuf, TMP_BUF_SEC_CNT))
            {
                TRACE("Initialize FAT fail!!!\n");
                ret = ERROR_WRITE_FAULT;
            }
            sectorCount += TMP_BUF_SEC_CNT;
        } 
        else
        {
            if (writeSectors((sector + sectorCount), tmpBuf, (ns - sectorCount)))
            {
                TRACE("Initialize FAT fail!!!\n");
                ret = ERROR_WRITE_FAULT;
            }
            sectorCount = ns;
        }
    }
#endif // _NORMAL_FORMAT

    // Write cluster two, bitmap.
    sector = clusterHeapOffset;
    bitmapSize = (clusterCount + 7) / 8;
    ns = (bitmapSize + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR;
    if (ns > sectorsPerCluster)
    {
        TRACE("Write cluster two, bitmap fail!!!\n");
        ret = ERROR_WRITE_FAULT;
    }

    // Allocate clusters for bitmap, upcase, and root.
    ZeroMemory(m_uPbr.ub, BYTES_PER_SECTOR);
    m_uPbr.ub[0] = 0X7;
    if (writeSectors(sector, m_uPbr.ub))
    {
        TRACE("Allocate clusters for bitmap fail\n");
        ret = ERROR_WRITE_FAULT;
    }

#ifdef _NORMAL_FORMAT
    sectorCount = 1;
    while (sectorCount < ns)
    {
        if ((ns - sectorCount) >= TMP_BUF_SEC_CNT)
        {
            if (writeSectors((sector + sectorCount), tmpBuf, TMP_BUF_SEC_CNT))
            {
                TRACE("Allocate clusters for bitmap fail\n");
                ret = ERROR_WRITE_FAULT;
            }
            sectorCount += TMP_BUF_SEC_CNT;
        }
        else
        {
            if (writeSectors((sector + sectorCount), tmpBuf, (ns - sectorCount)))
            {
                TRACE("Allocate clusters for bitmap fail\n");
                ret = ERROR_WRITE_FAULT;
            }
            sectorCount = ns;
        }
    }
#endif // _NORMAL_FORMAT

    // Write cluster three, upcase table.
    if (!writeUpcase(clusterHeapOffset + sectorsPerCluster))
    {
        TRACE("Writing upcase table fail\n");
        ret = ERROR_WRITE_FAULT;
    }
    if (m_upcaseSize > BYTES_PER_SECTOR * sectorsPerCluster)
    {
        TRACE("Writing upcase table fail\n");
        ret = ERROR_WRITE_FAULT;
    }

    // Initialize first sector of root.
    ns = sectorsPerCluster;
    sector = clusterHeapOffset + 2 * sectorsPerCluster;
    ZeroMemory(m_uPbr.ub, BYTES_PER_SECTOR);

    // Unused Label entry.
    DirLabel_t* label = reinterpret_cast<DirLabel_t*>(m_uPbr.ub);
    label->type = EXFAT_TYPE_LABEL & 0X7F;

    // bitmap directory  entry.
    DirBitmap_t* dbm = reinterpret_cast<DirBitmap_t*>(m_uPbr.ub + 32);
    dbm->type = EXFAT_TYPE_BITMAP;
    dbm->firstCluster = BITMAP_CLUSTER;
    dbm->size = bitmapSize;

    // upcase directory entry.
    DirUpcase_t* dup = reinterpret_cast<DirUpcase_t*>(m_uPbr.ub + 64);
    dup->type = EXFAT_TYPE_UPCASE;
    dup->checksum = m_upcaseChecksum;
    dup->firstCluster = UPCASE_CLUSTER;
    dup->size = m_upcaseSize;

    // Write root, cluster four.
    if (writeSectors(sector, m_uPbr.ub))
    {
        TRACE("Writing root fail\n");
        ret = ERROR_WRITE_FAULT;
    }
#ifdef _NORMAL_FORMAT
    sectorCount = 1;
    while (sectorCount < ns)
    {
        if ((ns - sectorCount) >= TMP_BUF_SEC_CNT)
        {
            if (writeSectors((sector + sectorCount), tmpBuf, TMP_BUF_SEC_CNT))
            {
                TRACE("Writing root fail\n");
                ret = ERROR_WRITE_FAULT;
            }
            sectorCount += TMP_BUF_SEC_CNT;
        }
        else
        {
            if (writeSectors((sector + sectorCount), tmpBuf, (ns - sectorCount)))
            {
                TRACE("Writing root fail\n");
                ret = ERROR_WRITE_FAULT;
            }
            sectorCount = ns;
        }
    }
#endif // _NORMAL_FORMAT

    GlobalFree(tmpBuf);

    return ret;
}

uint32_t ExFatVolume::exFatChecksum(uint32_t sum, uint8_t data)
{
    return (sum << 31) + (sum >> 1) + data;
}

bool ExFatVolume::writeUpcase(uint32_t sector)
{
    uint32_t n;
    uint32_t ns;
    uint32_t ch = 0;
    uint16_t uc;

    m_upcaseSize = 0;
    m_upcaseChecksum = 0;
    m_upcaseSector = sector;

    while (ch < 0X10000)
    {
        uc = toUpcase(ch);
        if (uc != ch)
        {
            if (!writeUpcaseUnicode(uc))
            {
                goto fail;
            }
            ch++;
        }
        else
        {
            for (n = ch + 1; n < 0X10000 && n == toUpcase(n); n++) {}
            ns = n - ch;
            if (ns >= MINIMUM_UPCASE_SKIP)
            {
                if (!writeUpcaseUnicode(0XFFFF) || !writeUpcaseUnicode(ns))
                {
                }
                ch = n;
            }
            else
            {
                while (ch < n)
                {
                    if (!writeUpcaseUnicode(ch++))
                    {
                    }
                }
            }
        }
    }
    if (!syncUpcase())
    {
    }
    return true;

fail:
    return false;
}

bool ExFatVolume::writeUpcaseUnicode(uint16_t unicode)
{
    return writeUpcaseByte(unicode) && writeUpcaseByte(unicode >> 8);
}

bool ExFatVolume::writeUpcaseByte(uint8_t b)
{
    uint16_t index = m_upcaseSize & SECTOR_MASK;
    m_uPbr.ub[index] = b;
    m_upcaseChecksum = exFatChecksum(m_upcaseChecksum, b);
    m_upcaseSize++;
    if (index == SECTOR_MASK)
    {
        return !writeSectors(m_upcaseSector++, m_uPbr.ub);
    }
    return true;
}

bool ExFatVolume::syncUpcase()
{
    uint16_t index = m_upcaseSize & SECTOR_MASK;
    if (!index)
    {
        return true;
    }
    for (size_t i = index; i < BYTES_PER_SECTOR; i++)
    {
        m_uPbr.ub[i] = 0;
    }

    return !writeSectors(m_upcaseSector, m_uPbr.ub);
}
