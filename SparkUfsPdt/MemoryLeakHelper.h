#pragma once
#ifdef _DEBUG
#include <string>
#include <unordered_map>
#include <crtdbg.h>

namespace MemoryLeakHelper
{
	// Save a named checkpoint of current CRT heap state
	void SaveCheckpoint(const std::string& name);

	// Dump differences between two named checkpoints into a file
	// Returns true on success
	bool DumpDiffToFile(const std::string& from, const std::string& to, const std::string& outPath);

	// Install / uninstall CRT report hook which captures CRT reports (including leak dumps)
	void InstallReportHook();
	void UninstallReportHook();
}
#endif
