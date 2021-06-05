#include "../framework/pch-sdk.h"
#include "main.h"
#include <iostream>
#include "../hooks/_hooks.h"
#include "logger.h"
#include "state.hpp"
#include <fstream>
#include <sstream>

HMODULE hModule;
HANDLE hUnloadEvent;

#define ToString(s) stringify(s)
#define stringify(s) #s

void Run(LPVOID lpParam) {
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	std::cout << "its working";

	//Log.Create();

	//hModule = (HMODULE)lpParam;

	//std::ostringstream ss;
	//ss << "\n\SeaOfThievesMenu - " << __DATE__ << " - " << __TIME__ << std::endl;
	////ss << "\tBuild: " << _CONFIGURATION_NAME_ << std::endl;
	//LOG_INFO(ss.str());

	//DetourInitilization();
#if _DEBUG
	DWORD dwWaitResult = WaitForSingleObject(hUnloadEvent, INFINITE);
	if (dwWaitResult != WAIT_OBJECT_0) {
		STREAM_ERROR("Failed to watch unload signal! dwWaitResult = " << dwWaitResult << " Error " << GetLastError());
		return;
	}
	
	DetourUninitialization();
	fclose(stdout);
	FreeConsole();
	CloseHandle(hUnloadEvent);
	FreeLibraryAndExitThread(hModule, 0);
#endif
}
