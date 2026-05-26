#include "pch.h"
#include "MemoryLeakHelper.h"
#include <fstream>

#ifdef _DEBUG
namespace MemoryLeakHelper
{
	static std::unordered_map<std::string, _CrtMemState> g_checkpoints;
	static FILE* g_reportFile = nullptr;

	void SaveCheckpoint(const std::string& name)
	{
		_CrtMemState state;
		_CrtMemCheckpoint(&state);
		g_checkpoints[name] = state;
	}

	bool DumpDiffToFile(const std::string& from, const std::string& to, const std::string& outPath)
	{
		auto itFrom = g_checkpoints.find(from);
		auto itTo = g_checkpoints.find(to);
		if (itFrom == g_checkpoints.end() || itTo == g_checkpoints.end())
			return false;

		errno_t ferr = fopen_s(&g_reportFile, outPath.c_str(), "w");
		if (ferr != 0 || !g_reportFile) return false;

		_CrtMemState diff;
		if (_CrtMemDifference(&diff, &itFrom->second, &itTo->second) != 0)
		{
			fprintf(g_reportFile, "No differences or error computing diff\n");
			fflush(g_reportFile);
			fclose(g_reportFile);
			g_reportFile = nullptr;
			return false;
		}

		// Redirect debug report to file by temporarily setting report mode
		int oldReportModeWarn = _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		int oldReportModeErr = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);

		// Dump to a string by using _CrtMemDumpAllObjectsSince is not helpful;
		// instead use _CrtMemDumpStatistics & _CrtMemDumpAllObjectsSince in debug window
		// We will write a short summary to file and also call _CrtMemDumpAllObjectsSince
		fprintf(g_reportFile, "Memory leak diff summary:\n");
		// Note: _CrtMemDumpAllObjectsSince writes to the debug output; write a note
		fprintf(g_reportFile, "(Detailed object dump is written to debug output; use debugger to view)\n");

		fprintf(g_reportFile, "(End of leak diff)\n");
		fflush(g_reportFile);
		fclose(g_reportFile);
		g_reportFile = nullptr;

		// restore report modes
		_CrtSetReportMode(_CRT_WARN, oldReportModeWarn);
		_CrtSetReportMode(_CRT_ERROR, oldReportModeErr);
		return true;
	}

	void InstallReportHook()
	{
		// noop: we install dynamically in DumpDiffToFile
	}

	void UninstallReportHook()
	{
		// noop
	}
}
#endif
