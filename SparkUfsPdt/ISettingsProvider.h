#pragma once
#include "libsparkusb.h"
#include "CDialogBase.h"

// Abstract interface to provide access to UFS settings/options used by business logic.
class ISettingsProvider
{
public:
	virtual ~ISettingsProvider() = default;
	// Return pointer to PUFS_OPTION as used across codebase. May be nullptr.
	virtual PUFS_OPTION GetUfsOption() const = 0;
	virtual void SaveDefaultFactoryCom(const CString& comName) = 0;
	// Return pointer to shared base setting. May be nullptr.
	virtual PST_UFS_BASE_SETTING GetBaseSetting() const = 0;
};
