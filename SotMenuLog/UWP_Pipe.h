#pragma once

#include <Windows.h>

#include <iostream>

// for UWP / ACL / token stuff
#include <sddl.h>

BOOL CreatePipeWithProperACL(PCWSTR PipeName, ULONG dwProcessId, OUT PHANDLE PipeHandle, OUT DWORD* errCode);