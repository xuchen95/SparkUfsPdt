// DataFormatter: pure data encoding utilities without UI dependency
#pragma once
#include <array>
#include <string>
#include <cstdint>

using BYTE = unsigned char;

namespace spark { namespace ufspdt {

class DataFormatter
{
public:
	// Format SN payload (64 bytes) given meto1(4 bytes), meto2(4 bytes), timeStr (WCHAR[9] null-terminated, YYYYMMDD), psn (WCHAR up to 8 chars)
	static std::array<BYTE, 64> FormatSnData(const char meto1[4], const char meto2[4], const WCHAR* timeStr, const WCHAR* psn);

	// Convert MDT hex string (like "0412") to 2-byte array (little-endian as used by existing code via HexToBytes)
	static std::array<BYTE, 2> FormatMdtFromHex(const char* hexStr);

	// Encode ISP mark string (16 chars) into 12-byte isp mark as used by VerifyIsp
	// Returns empty optional (all zeros) on parse failure
	static std::array<BYTE, 12> EncodeIspMark(const char* ispMark16);
	static std::array<BYTE, 12> EncodeIspMark2(const char* ispMark16);
};

}} // namespace
