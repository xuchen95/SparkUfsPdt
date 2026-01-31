#pragma once
class CSm3350Vcmds;

template <class C, typename T>
class Sm3350Volume : public C
{
public:
    Sm3350Volume(T pSm3350);

    int readSectors(uint32_t sector, uint8_t* dest, size_t ns = 1) override;
    int writeSectors(uint32_t sector, const uint8_t* src, size_t ns = 1) override;

private:
    T m_pSm3350 = nullptr;
};

template <class C, typename T>
int Sm3350Volume<C, T>::writeSectors(uint32_t sector, const uint8_t* src, size_t ns /*= 1*/)
{
    return m_pSm3350->SendWriteCmd(sector, ns, (PCHAR)src);
}

template <class C, typename T>
int Sm3350Volume<C, T>::readSectors(uint32_t sector, uint8_t* dest, size_t ns /*= 1*/)
{
    return m_pSm3350->SendReadCmd(sector, ns, (PCHAR)dest);
}

template <class C, typename T>
Sm3350Volume<C, T>::Sm3350Volume(T pSm3350)
{
    m_pSm3350 = pSm3350;
}
