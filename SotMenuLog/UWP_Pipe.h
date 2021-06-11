#pragma once

#include <Windows.h>

#include <iostream>

// for UWP / ACL / token stuff
#include <sddl.h>
#define RtlPointerToOffset(B,P)  ((ULONG)(((PCHAR)(P)) - ((PCHAR)(B))))

BOOL CreatePipeforUWP(PCWSTR PipeName, ULONG dwProcessId, OUT PHANDLE PipeHandle, OUT DWORD* errCode);