#include "pch.h"
#include "IspMarkCache.h"
#include "DataFormatter.h"

using namespace spark::ufspdt;

IspMarkCache& IspMarkCache::Instance()
{
	static IspMarkCache s;
	return s;
}

void IspMarkCache::Update(const char* ispBuf, int ispFileSize)
{
	std::unique_lock<std::shared_mutex> lock(mutex_);
	if (ispBuf == nullptr || ispFileSize < ISP_MARK_SIZE)
	{
		valid_ = false;
		return;
	}
	memcpy(mark_.data(), ispBuf + ispFileSize - ISP_MARK_SIZE, ISP_MARK_SIZE);
	valid_ = true;
}

bool IspMarkCache::GetRawMark(char* outBuf, size_t outLen) const
{
	if (outBuf == nullptr || outLen < ISP_MARK_SIZE) return false;
	std::shared_lock<std::shared_mutex> lock(mutex_);
	if (!valid_) return false;
	memcpy(outBuf, mark_.data(), ISP_MARK_SIZE);
	return true;
}

bool IspMarkCache::GetEncodedMark(unsigned char* outBuf, size_t outLen) const
{
	if (outBuf == nullptr || outLen < 12) return false;
	char raw[ISP_MARK_SIZE + 1] = {0};
	if (!GetRawMark(raw, sizeof(raw))) return false;
	auto encoded = DataFormatter::EncodeIspMark(raw);
	memcpy(outBuf, encoded.data(), 12);
	return true;
}
