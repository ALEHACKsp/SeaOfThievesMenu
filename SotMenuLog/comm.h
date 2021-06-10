#pragma once
#include <iostream>
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <wchar.h>
#include <filesystem>
#include <iostream>
#include <aclapi.h>
#include <sddl.h>
#include <comdef.h>

#define RtlPointerToOffset(B,P)  ((ULONG)(((PCHAR)(P)) - ((PCHAR)(B))))
#define BOOL_TO_ERROR(f) f ? NOERROR : GetLastError();

ULONG CreatePipeFromProcess(OUT PHANDLE PipeHandle, PCWSTR PipeName, HANDLE hProcess);
DWORD GetProcessIdByName(const char* name);
ULONG CreatePipeFromPid(OUT PHANDLE PipeHandle, PCWSTR PipeName, ULONG dwProcessId);