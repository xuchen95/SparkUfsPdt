#pragma once
#include <string>

class ILogger
{
public:
	virtual ~ILogger() = default;
	virtual void LogLine(const std::string& line) = 0;
};
