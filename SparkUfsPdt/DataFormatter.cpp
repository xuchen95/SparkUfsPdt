#include "pch.h"
#include "DataFormatter.h"
#include "PubFunc.h"

using namespace spark::ufspdt;

std::array<BYTE, 64> DataFormatter::FormatSnData(const char meto1[4], const char meto2[4], const WCHAR* timeStr, const WCHAR* psn)
{
	std::array<BYTE, 64> out = {0};
	out[0] = 0x40;
	out[1] = 0x05;
	int offset = 2;
	if (meto1)
	{
		memcpy(out.data() + offset, meto1, 4);
	}
	offset += 4;

	if (timeStr)
	{
		// copy as bytes (WCHAR) into buffer to keep compatibility with original layout
		memcpy(out.data() + offset, timeStr, 16);
	}
	offset += 16;

	// psn: WCHAR up to 9 (including null)
	if (psn)
	{
		memcpy(out.data() + offset, psn, 16);
	}
	offset += 16;
	if(meto2)
	{
		memcpy(out.data() + offset, meto2, 4);
	}
	offset += 4;
	// rest: fill with alternating 0x00, 0x20 as original
	for (int i = offset; i < 60; i += 2)
	{
		out[i] = 0x00;
		if (i + 1 < 64) out[i + 1] = 0x20;
	}
	return out;
}

std::array<BYTE, 2> DataFormatter::FormatMdtFromHex(const char* hexStr)
{
	std::array<BYTE,2> out = {0,0};
	if (hexStr == nullptr) return out;
	BYTE buf[2] = {0};
	int ret = CPubFunc::HexToBytes(hexStr, buf, sizeof(buf));
	if (ret > 0)
	{
		out[0] = buf[0];
		out[1] = buf[1];
	}
	return out;
}

std::array<BYTE, 12> DataFormatter::EncodeIspMark(const char* ispMark16)
{
	std::array<BYTE, 12> out = {0};
	if (ispMark16 == nullptr) return out;

	// reuse existing encode logic but safer
	char part2Str[5] = {0};
	char part3Str[9] = {0};
	memcpy(part2Str, ispMark16 + 4, 4);
	memcpy(part3Str, ispMark16 + 8, 8);

	for (int i = 0; i < 4; ++i) if (!isdigit((unsigned char)part2Str[i])) return out;
	for (int i = 0; i < 8; ++i) if (!isdigit((unsigned char)part3Str[i])) return out;

	unsigned long part2Value = strtoul(part2Str, nullptr, 10);
	unsigned long part3Value = strtoul(part3Str, nullptr, 10);
	if (part2Value > 0xFFFFUL || part3Value > 0xFFFFFFFFUL) return out;

	out[0] = static_cast<BYTE>(part2Value & 0xFF);
	out[1] = static_cast<BYTE>((part2Value >> 8) & 0xFF);
	out[2] = 0x00;
	out[3] = 0x00;
	out[4] = static_cast<BYTE>(part3Value & 0xFF);
	out[5] = static_cast<BYTE>((part3Value >> 8) & 0xFF);
	out[6] = static_cast<BYTE>((part3Value >> 16) & 0xFF);
	out[7] = static_cast<BYTE>((part3Value >> 24) & 0xFF);
	memcpy(out.data() + 8, ispMark16, 4);
	return out;
}

std::array<BYTE, 12> spark::ufspdt::DataFormatter::EncodeIspMark2(const char* ispMark16)
{
	std::array<BYTE, 12> out = { 0 };
	memcpy(out.data(), ispMark16, out.size());
	return out;
}
