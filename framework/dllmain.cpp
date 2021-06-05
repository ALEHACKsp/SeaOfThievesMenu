#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../user/main.h"

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinst);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Run, hinst, NULL, NULL);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}