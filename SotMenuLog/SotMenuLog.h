#pragma once

#include "UWP_Pipe.h"
#include "utils.h"

#include <iostream>

#include <Windows.h>

#define PIPE_TIMEOUT 5000
#define BUFSIZE 4096

typedef struct
{
    OVERLAPPED oOverlap;
    HANDLE hPipeInst;
    char buffer[BUFSIZE];
    DWORD bytesRead;
} PIPEINST, * LPPIPEINST;

VOID DisconnectAndClose(LPPIPEINST);
BOOL CreateAndConnectInstance(LPOVERLAPPED);
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED);

VOID WINAPI BeginRead(LPOVERLAPPED lpOverLap);
VOID WINAPI CompletedReadRoutine(DWORD, DWORD, LPOVERLAPPED);