#pragma once
#include "../includes/directx11.h"
#include <vector>

extern D3D_PRESENT_FUNCTION oPresent;
HRESULT __stdcall dPresent(IDXGISwapChain* __this, UINT SyncInterval, UINT Flags);

namespace DirectX {
	extern HANDLE hRenderSemaphore;
	void Shutdown();
}