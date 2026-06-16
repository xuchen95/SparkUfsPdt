#pragma once
#include "ISettingsProvider.h"

class SettingsService : public ISettingsProvider
{
public:
	static SettingsService& Instance();

	// ISettingsProvider
	PUFS_OPTION GetUfsOption() const override;
	void SaveDefaultFactoryCom(const CString& comName) override;
	PST_UFS_BASE_SETTING GetBaseSetting() const override;

	// Persistence helpers
	void LoadBaseSettingFromIni(const CString& path);
	void SaveBaseSettingToIni(const CString& path);
	void LoadRemoteSnToMainParam();
	CString GetIspPath() const;
private:
	SettingsService();
	~SettingsService() = default;

	// non-copyable
	SettingsService(const SettingsService&) = delete;
	SettingsService& operator=(const SettingsService&) = delete;

	UFS_OPTION sharedOption_ = {};
	ST_UFS_BASE_SETTING baseOption_ = {};
};