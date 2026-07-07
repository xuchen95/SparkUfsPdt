#pragma once
#include <string>

class IUiNotifier
{
public:
	virtual ~IUiNotifier() = default;
	virtual void PostTaskProgress(int portIndex, int progress, int result, const CString& status) = 0;
	// Status-only helper: does not carry numeric progress, used for stage entry and failures
	virtual void PostTaskStatus(int portIndex, int result, const CString& status) = 0;
	// Update a port's SN text in the device list.
	virtual void PostPortSerial(int portIndex, const CString& serial) = 0;
	// Update a port's temp text in the device list.
	virtual void PostPortTemp(int portIndex, const CString& temp) = 0;
};
