#pragma once
#include "ISettingsProvider.h"
#include "ILogger.h"
#include "IUiNotifier.h"
#include "SparkUfsPdtDlg.h"
#include "CDialogBase.h"
#include "SettingsService.h"

class DialogAdapter : public ISettingsProvider, public ILogger, public IUiNotifier
{
public:
	explicit DialogAdapter(CSparkUfsPdtDlg* dlg) : dlg_(dlg), settings_(nullptr) {}

	// Alternative constructor to accept explicit settings provider (for injection)
	explicit DialogAdapter(ISettingsProvider* settings) : dlg_(nullptr), settings_(settings) {}

	// ISettingsProvider
	// Now DialogAdapter maps UI to the central SettingsService by default,
	// but if constructed with a dialog pointer it will prefer the dialog instance for instance-level data.
	PUFS_OPTION GetUfsOption() const override
	{
		if (dlg_) return dlg_->GetUfsOption();
		if (settings_) return settings_->GetUfsOption();
		return SettingsService::Instance().GetUfsOption();
	}
	void SaveDefaultFactoryCom(const CString& comName) override
	{
		if (comName.IsEmpty()) return;
		// Access shared base setting through the dialog's private static via friendship
		PST_UFS_BASE_SETTING pBase = settings_ ? settings_->GetBaseSetting() : SettingsService::Instance().GetBaseSetting();
		if (pBase)
		{
			CStringA comA(comName);
			strncpy_s(pBase->szComName, sizeof(pBase->szComName), comA, _TRUNCATE);
		}
	}

	PST_UFS_BASE_SETTING GetBaseSetting() const override
	{
		if (dlg_) return dlg_->GetBaseSetting();
		if (settings_) return settings_->GetBaseSetting();
		return SettingsService::Instance().GetBaseSetting();
	}

	// ILogger
	void LogLine(const std::string& line) override { CSparkUfsPdtDlg::AppendLogLine(CString(line.c_str())); }

	// IUiNotifier
	void PostTaskProgress(int portIndex, int progress, int result, const CString& status) override
	{
		if (!dlg_) return;
		CSparkUfsPdtDlg::TaskProgressMsg* pmsg = new CSparkUfsPdtDlg::TaskProgressMsg{portIndex, progress, result, status};
		dlg_->PostMessage(CSparkUfsPdtDlg::WM_TASK_PROGRESS, (WPARAM)pmsg, 0);
	}

private:
	CSparkUfsPdtDlg* dlg_ = nullptr;
	ISettingsProvider* settings_ = nullptr;
};
