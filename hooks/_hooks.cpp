#include "../framework/pch-sdk.h"
#include "_hooks.h"
#include "../includes/detours/detours.h"
#include "DirectX.h"
#include <iostream>
#include "../user/main.h"
#include "SignatureScan.hpp"

bool HookFunction(PVOID* ppPointer, PVOID pDetour, const char* functionName) {
	if (DetourAttach(ppPointer, pDetour) != 0) {
		std::cout << "Failed to hook " << functionName << std::endl;
		return false;
	}
	std::cout << "Hooked " << functionName << std::endl;
	return true;
}

#define HOOKFUNC(n) if (!HookFunction(&(PVOID&)n, d ## n, #n)) return;

bool UnhookFunction(PVOID* ppPointer, PVOID pDetour, const char* functionName) {
	if (DetourDetach(ppPointer, pDetour) != 0) {
		std::cout << "Failed to unhook " << functionName << std::endl;
		return false;
	}
	std::cout << "Unhooked " << functionName << std::endl;
	return true;
}

#define UNHOOKFUNC(n) if (!UnhookFunction(&(PVOID&)n, d ## n, #n)) return;

void DetourInitilization() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	directx11 d3d11 = directx11();
	if (!d3d11.presentFunction) {
		std::cout << "Unable to retrieve IDXGISwapChain::Present method" << std::endl;
		return;
	}
	else {
		oPresent = d3d11.presentFunction;
	}

	if (!HookFunction(&(PVOID&)oPresent, dPresent, "D3D_PRESENT_FUNCTION")) return;

	DetourTransactionCommit();
}

void DetourUninitialization()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	
	if (DetourDetach(&(PVOID&)oPresent, dPresent) != 0) return;

	DetourTransactionCommit();
	DirectX::Shutdown();
}