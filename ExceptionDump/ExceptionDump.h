#pragma once

#include <windows.h>
#include <string>

// Install a top-level exception handler that writes a minidump file when an
// unhandled exception occurs. If 'prefix' is empty, the file will be named
// "ExceptionDump_YYYYMMDD_HHMMSS.dmp" and placed in the current working
// directory.
void InstallExceptionDump(const std::string& prefix = "");

// Uninstall the previously installed handler.
void UninstallExceptionDump();

// Create a minidump for the given exception pointers and path. Returns true
// on success.
bool CreateMiniDump(EXCEPTION_POINTERS* exinfo, const std::string& dumpPath);
