#pragma once
#include "ISettingsProvider.h"
#include "ILogger.h"
#include "IUiNotifier.h"
#include "SparkUfsPdtDlg.h"
#include "CDialogBase.h"
#include "SettingsService.h"
#include "EventBus.h"
#include "EventMessages.h"

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
		// Publish event to EventBus for the dialog to consume on UI thread.
		spark::ufspdt::ProgressEvent evt;
		evt.type = spark::ufspdt::ProgressEvent::EventType::Progress;
		evt.portIndex = portIndex;
		evt.progress = progress;
		evt.result = result;
		// If this is a failure, ensure status is formatted consistently
		CString sendStatus = status;
		if (result != 0)
		{
			sendStatus = FormatFailureStatus(status, result);
		}
		// convert CString (TCHAR) to UTF-8 std::string
		CT2A conv(sendStatus, CP_UTF8);
		evt.status = std::string(conv);
		// Use the dialog window handle as the target
		spark::ufspdt::EventBus::Instance().Publish(dlg_->GetSafeHwnd(), evt);

		// If this progress represents completion (100%) post UI commands for counts and UI update
	// Note: Do not alter active task counts here. Task lifecycle accounting is handled
	// centrally at task entry/exit to avoid duplicate or missing increments/decrements.
	}

	// Post a UI-level command (intent) to be executed on the dialog's UI thread
	void PostUiCommand(spark::ufspdt::UICommand cmd, int portIndex = -1, int value = 0, const CString& text = CString())
	{
		if (!dlg_) return;
		spark::ufspdt::UIEvent u;
		u.cmd = cmd;
		u.portIndex = portIndex;
		u.value = value;
		CT2A conv(text, CP_UTF8);
		u.text = std::string(conv);
		// Use EventBus PublishUI so worker threads can also post via HWND.
		spark::ufspdt::EventBus::Instance().PublishUI(dlg_->GetSafeHwnd(), u);
	}

	void PostTaskStatus(int portIndex, int result, const CString& status) override
	{
		if (!dlg_) return;
		spark::ufspdt::ProgressEvent evt;
		evt.type = spark::ufspdt::ProgressEvent::EventType::StatusOnly;
		evt.portIndex = portIndex;
		evt.progress = -1;
		evt.result = result;
		// Always format failures consistently
		CString sendStatus = status;
		if (result != 0)
		{
			sendStatus = FormatFailureStatus(status, result);
		}
		CT2A conv(sendStatus, CP_UTF8);
		evt.status = std::string(conv);
		spark::ufspdt::EventBus::Instance().Publish(dlg_->GetSafeHwnd(), evt);

		// Post UI commands for failure: decrement active tasks and increment fail count
	// Only post counts/status; do not touch active task counter here. The worker
	// invocation is responsible for decrementing the active task count when it exits.
	PostUiCommand(spark::ufspdt::UICommand::IncrementFailCount, portIndex, 1, CString());
	PostUiCommand(spark::ufspdt::UICommand::UpdateStatusBar, portIndex, 0, CString());
	}

	// Format a status string consistently for failures: "<Stage> Failed (0xXXXX)"
	static CString FormatFailureStatus(const CString& stage, int errCode)
	{
		CString base = stage;
		base.Trim();
		// Remove trailing "Failed" (case-insensitive) if already present
		CString up = base;
		up.MakeUpper();
		if (up.GetLength() >= 6 && up.Right(6) == _T("FAILED"))
		{
			base = base.Left(base.GetLength() - 6);
			base.TrimRight();
		}

		// If the stage contains no spaces, try to split CamelCase into words
		if (base.Find(_T(' ')) == -1)
		{
			CString tmp = base;
			CString spaced;
			for (int i = 0; i < tmp.GetLength(); ++i)
			{
				TCHAR ch = tmp[i];
				if (i > 0 && _istupper(ch) && !_istupper(tmp[i-1]) && !_istspace(tmp[i-1]))
				{
					spaced.AppendChar(' ');
				}
				spaced.AppendChar(ch);
			}
			spaced.Trim();
			if (!spaced.IsEmpty()) base = spaced;
		}
		CString s;
		// Use lowercase '0x' and uppercase hex letters for consistency
		s.Format(_T("%s Failed (0x%X)"), base.GetString(), errCode);
		return s;
	}

private:
	CSparkUfsPdtDlg* dlg_ = nullptr;
	ISettingsProvider* settings_ = nullptr;
};
