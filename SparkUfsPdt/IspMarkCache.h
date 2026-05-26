#pragma once
#include <shared_mutex>
#include <array>

namespace spark { namespace ufspdt {

class IspMarkCache
{
public:
	static IspMarkCache& Instance();

	// Update cache with ispBuf contents (will copy last ISP_MARK_SIZE bytes)
	void Update(const char* ispBuf, int ispFileSize);

	// Get raw 16-byte mark; returns true if valid
	bool GetRawMark(char* outBuf, size_t outLen) const;

	// Get encoded 12-byte mark (as used by VerifyIsp); returns true if valid
	bool GetEncodedMark(unsigned char* outBuf, size_t outLen) const;

private:
	IspMarkCache() = default;
	static constexpr int ISP_MARK_SIZE = 16;
	mutable std::shared_mutex mutex_;
	std::array<char, ISP_MARK_SIZE> mark_{};
	bool valid_ = false;
};

}} // namespace
