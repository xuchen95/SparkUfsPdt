#pragma once
#include <string>

class IUiNotifier
{
public:
	virtual ~IUiNotifier() = default;
	virtual void PostTaskProgress(int portIndex, int progress, int result, const CString& status) = 0;
};
