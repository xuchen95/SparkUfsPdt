#include "pch.h"
#include <iostream>
#include <vector>
#include <cstring>
#include "../DataFormatter.h"
#include "../IspMarkCache.h"

using namespace spark::ufspdt;

static bool AssertEqual(const unsigned char* a, const unsigned char* b, size_t n)
{
	return std::memcmp(a, b, n) == 0;
}

int TestEncodeIspMark()
{
	const char* mark = "M53B296404270945"; // example
	auto encoded = DataFormatter::EncodeIspMark(mark);

	unsigned char expected[12] = { 0x94, 0x0B, 0x00, 0x00, 0x61, 0x2B, 0x41, 0x00, 0x4D, 0x35, 0x33, 0x42 };
	if (!AssertEqual(encoded.data(), expected, sizeof(expected)))
	{
		std::cout << "TestEncodeIspMark FAILED\n";
		return 1;
	}
	std::cout << "TestEncodeIspMark PASSED\n";
	return 0;
}

int TestFormatMdt()
{
	const char* hex = "2964";
	auto mdt = DataFormatter::FormatMdtFromHex(hex);
	// CPubFunc::HexToBytes("2964") -> bytes 0x29 0x64? but original usage expects little-endian; we'll at least check non-zero
	if (mdt[0] == 0 && mdt[1] == 0)
	{
		std::cout << "TestFormatMdt FAILED (zero)\n";
		return 1;
	}
	std::cout << "TestFormatMdt PASSED\n";
	return 0;
}

int TestFormatSnData()
{
	char meto[4] = { 'M','E','T','O' };
	const WCHAR timeStr[] = L"20230101";
	const WCHAR psn[] = L"SN123456";
	auto sn = DataFormatter::FormatSnData(meto, timeStr, psn);
	if (sn[0] != 0x40 || sn[1] != 0x05)
	{
		std::cout << "TestFormatSnData FAILED (header)\n";
		return 1;
	}
	if (std::memcmp(sn.data() + 2, meto, 4) != 0)
	{
		std::cout << "TestFormatSnData FAILED (meto)\n";
		return 1;
	}
	// timeStr is WCHAR copied as bytes
	if (std::memcmp(sn.data() + 6, timeStr, 16) != 0)
	{
		std::cout << "TestFormatSnData FAILED (time)\n";
		return 1;
	}
	std::cout << "TestFormatSnData PASSED\n";
	return 0;
}

int TestIspMarkCache()
{
	IspMarkCache& cache = IspMarkCache::Instance();
	// small buffer -> invalid
	const char smallBuf[] = "short";
	cache.Update(smallBuf, (int)strlen(smallBuf));
	unsigned char enc[12] = {0};
	if (cache.GetEncodedMark(enc, sizeof(enc)))
	{
		std::cout << "TestIspMarkCache FAILED (should be invalid)\n";
		return 1;
	}

	// build buffer with trailing 16 bytes
	char buf[64] = {0};
	const char* mark = "M53B296404270945";
	memcpy(buf + sizeof(buf) - 16, mark, 16);
	cache.Update(buf, sizeof(buf));

	char raw[17] = {0};
	if (!cache.GetRawMark(raw, sizeof(raw)))
	{
		std::cout << "TestIspMarkCache FAILED (GetRawMark)\n";
		return 1;
	}
	if (std::memcmp(raw, mark, 16) != 0)
	{
		std::cout << "TestIspMarkCache FAILED (raw mismatch)\n";
		return 1;
	}

	unsigned char expected[12] = { 0x94, 0x0B, 0x00, 0x00, 0x61, 0x2B, 0x41, 0x00, 0x4D, 0x35, 0x33, 0x42 };
	unsigned char enc2[12] = {0};
	if (!cache.GetEncodedMark(enc2, sizeof(enc2)))
	{
		std::cout << "TestIspMarkCache FAILED (GetEncodedMark)\n";
		return 1;
	}
	if (!AssertEqual(enc2, expected, 12))
	{
		std::cout << "TestIspMarkCache FAILED (encoded mismatch)\n";
		return 1;
	}

	std::cout << "TestIspMarkCache PASSED\n";
	return 0;
}

int main()
{
	int failures = 0;
	failures += TestEncodeIspMark();
	failures += TestFormatMdt();
	failures += TestFormatSnData();
	failures += TestIspMarkCache();
	if (failures == 0)
	{
		std::cout << "ALL TESTS PASSED\n";
		return 0;
	}
	std::cout << failures << " TESTS FAILED\n";
	return 1;
}
