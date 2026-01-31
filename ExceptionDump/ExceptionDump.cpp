// ExceptionDump.cpp : 定义静态库的函数。
//

#include "pch.h"
#include "ExceptionDump.h"

#include <dbghelp.h>
#include <chrono>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "dbghelp.lib")

static LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter = nullptr;
static std::string g_prefix;

static std::string GetTimestampString()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _MSC_VER
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

bool CreateMiniDump(EXCEPTION_POINTERS* exinfo, const std::string& dumpPath)
{
    HANDLE hFile = CreateFileA(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = exinfo;
    mei.ClientPointers = FALSE;

    BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, nullptr, nullptr);
    CloseHandle(hFile);
    return ok == TRUE;
}

static LONG WINAPI TopLevelHandler(EXCEPTION_POINTERS* exinfo)
{
    std::string name = g_prefix.empty() ? "ExceptionDump_" + GetTimestampString() + ".dmp" : g_prefix + "_" + GetTimestampString() + ".dmp";
    CreateMiniDump(exinfo, name);
    if (g_prevFilter)
        return g_prevFilter(exinfo);
    return EXCEPTION_EXECUTE_HANDLER;
}

void InstallExceptionDump(const std::string& prefix)
{
    g_prefix = prefix;
    g_prevFilter = SetUnhandledExceptionFilter(TopLevelHandler);
}

void UninstallExceptionDump()
{
    SetUnhandledExceptionFilter(g_prevFilter);
    g_prevFilter = nullptr;
    g_prefix.clear();
}
#include "framework.h"

