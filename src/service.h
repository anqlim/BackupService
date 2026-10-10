#pragma once

#include <windows.h>
#include <tchar.h>

inline constexpr TCHAR kServiceName[] = _T("BackupService");

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
