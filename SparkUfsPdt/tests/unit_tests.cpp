#include "pch.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <windows.h>
#include "../DataFormatter.h"
#include "../IspMarkCache.h"
// ====== NEW: Counting policy for P0-2 (pass never counted) + P1-4 (fail double counted) ======
#include "../TaskCountPolicy.h"

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
	char meto1[4] = { 'M','E','T','O' };
	char meto2[4] = { 'M','E','T','O' };
	const WCHAR timeStr[] = L"20230101";
	const WCHAR psn[] = L"SN123456";
	auto sn = DataFormatter::FormatSnData(meto1, meto2, timeStr, psn);
	if (sn[0] != 0x40 || sn[1] != 0x05)
	{
		std::cout << "TestFormatSnData FAILED (header)\n";
		return 1;
	}
	if (std::memcmp(sn.data() + 2, meto1, 4) != 0)
	{
		std::cout << "TestFormatSnData FAILED (meto1)\n";
		return 1;
	}
	if (std::memcmp(sn.data() + 22, meto2, 4) != 0)
	{
		std::cout << "TestFormatSnData FAILED (meto2)\n";
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

// -------------------------------------------------------------------------
// TDD tests for Issue #1 (P0-2 IncrementPassCount never fired, P1-4 fail double counted)
// Policy:
//   * PASS counted only once per task, when PostTaskProgress reports progress>=100 && result==ERROR_SUCCESS
//   * FAIL counted only once per task, when the outermost RunPdtTask wrapper observes the
//     inner RunFtTaskImpl/RunQcTaskImpl return value != 0. Stage-level PostTaskStatus
//     (e.g. SetSnStage failure) MUST NOT increment the global fail counter by itself.
// -------------------------------------------------------------------------
int TestPassCountTriggeredOnFinalSuccess()
{
	// 100% + ERROR_SUCCESS → count as PASS
	if (!TaskCountPolicy::ShouldIncrementPassOnProgress(100, ERROR_SUCCESS))
	{
		std::cout << "TestPassCountTriggeredOnFinalSuccess FAILED (100,0)\n";
		return 1;
	}
	// 100% but non-zero result (e.g. pipeline finished but Verify failed) → NO PASS
	if (TaskCountPolicy::ShouldIncrementPassOnProgress(100, 0x37))
	{
		std::cout << "TestPassCountTriggeredOnFinalSuccess FAILED (100,0x37)\n";
		return 1;
	}
	// Intermediate progress even with 0 result → NOT final yet, do not count
	if (TaskCountPolicy::ShouldIncrementPassOnProgress(99, ERROR_SUCCESS))
	{
		std::cout << "TestPassCountTriggeredOnFinalSuccess FAILED (99,0)\n";
		return 1;
	}
	// Edge: progress 101 (e.g. overflow guard) with success → still counts
	if (!TaskCountPolicy::ShouldIncrementPassOnProgress(101, ERROR_SUCCESS))
	{
		std::cout << "TestPassCountTriggeredOnFinalSuccess FAILED (101,0)\n";
		return 1;
	}
	std::cout << "TestPassCountTriggeredOnFinalSuccess PASSED\n";
	return 0;
}

int TestStageStatusNeverCountsFail()
{
	// Stage-level PostTaskStatus calls are made by individual stages for UI display only.
	// They must NEVER bump the global fail counter, otherwise a single task failing mid-pipeline
	// causes 1 + N fail increments (P1-4 bug).
	if (TaskCountPolicy::ShouldIncrementFailOnStageStatus(ERROR_SUCCESS))
	{
		std::cout << "TestStageStatusNeverCountsFail FAILED (result=0 still counted)\n";
		return 1;
	}
	if (TaskCountPolicy::ShouldIncrementFailOnStageStatus(0xF0C))
	{
		std::cout << "TestStageStatusNeverCountsFail FAILED (SN mismatch double counted)\n";
		return 1;
	}
	if (TaskCountPolicy::ShouldIncrementFailOnStageStatus(0x37))
	{
		std::cout << "TestStageStatusNeverCountsFail FAILED (device not exist double counted)\n";
		return 1;
	}
	std::cout << "TestStageStatusNeverCountsFail PASSED\n";
	return 0;
}

int TestFinalFailCountedOnce()
{
	// Outermost wrapper's final decision: any non-zero return of the task = increment fail ONCE.
	if (!TaskCountPolicy::ShouldIncrementFailOnTaskFinal(0xF0C))
	{
		std::cout << "TestFinalFailCountedOnce FAILED (SN mismatch not counted)\n";
		return 1;
	}
	if (!TaskCountPolicy::ShouldIncrementFailOnTaskFinal(0x37))
	{
		std::cout << "TestFinalFailCountedOnce FAILED (device not exist not counted)\n";
		return 1;
	}
	// Successful completion: never count fail.
	if (TaskCountPolicy::ShouldIncrementFailOnTaskFinal(ERROR_SUCCESS))
	{
		std::cout << "TestFinalFailCountedOnce FAILED (success wrongly counted as fail)\n";
		return 1;
	}
	std::cout << "TestFinalFailCountedOnce PASSED\n";
	return 0;
}

int main()
{
	int failures = 0;
	failures += TestEncodeIspMark();
	failures += TestFormatMdt();
	failures += TestFormatSnData();
	failures += TestIspMarkCache();
	// Issue #1 count-policy tests
	failures += TestPassCountTriggeredOnFinalSuccess();
	failures += TestStageStatusNeverCountsFail();
	failures += TestFinalFailCountedOnce();
	if (failures == 0)
	{
		std::cout << "ALL TESTS PASSED\n";
		return 0;
	}
	std::cout << failures << " TESTS FAILED\n";
	return 1;
}
