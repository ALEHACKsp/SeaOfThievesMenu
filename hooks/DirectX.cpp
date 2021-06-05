#include "../framework/pch-sdk.h"
#include "DirectX.h"
#include "../gui/Renderer.hpp"
#include "../includes/imgui/imgui.h"
#include "../includes/imgui/imgui_internal.h"
#include "../includes/imgui/imgui_impl_dx11.h"
#include "../includes/imgui/imgui_impl_win32.h"
#include "../user/keybinds.h"
#include "../gui/menu.hpp"
#include "../user/state.hpp"
#include "../gui/theme.hpp"
#include <mutex>
#include "../user/logger.h"

#include <future>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND window;
ID3D11Device* pDevice;
ID3D11DeviceContext* pContext;
ID3D11RenderTargetView* pRenderTargetView;
D3D_PRESENT_FUNCTION oPresent;
WNDPROC oWndProc;

HANDLE DirectX::hRenderSemaphore;
constexpr DWORD MAX_RENDER_THREAD_COUNT = 5; //Should be overkill for our purposes

typedef struct Cache
{
	ImGuiWindow* Window;  //Window instance
	ImVec2       Winsize; //Size of the window
} cache_t;

static cache_t s_Cache;


LRESULT __stdcall dWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (!State.ImGuiInitialized)
        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);

    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    KeyBinds::WndProc(uMsg, wParam, lParam);

    if (KeyBinds::IsKeyPressed(State.KeyBinds.Toggle_Menu)) State.ShowMenu = !State.ShowMenu;
  
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool ImGuiInitialization(IDXGISwapChain* pSwapChain) {
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
        pDevice->GetImmediateContext(&pContext);
        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);
        window = sd.OutputWindow;
        ID3D11Texture2D* pBackBuffer = NULL;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
        pBackBuffer->Release();
        oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)dWndProc);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX11_Init(pDevice, pContext);

        DirectX::hRenderSemaphore = CreateSemaphore(
            NULL,                                 // default security attributes
            MAX_RENDER_THREAD_COUNT,              // initial count
            MAX_RENDER_THREAD_COUNT,              // maximum count
            NULL);                                // unnamed semaphore);

        return true;
    }
    
    return false;
}

std::once_flag init_d3d;
HRESULT __stdcall dPresent(IDXGISwapChain* __this, UINT SyncInterval, UINT Flags) {
    std::call_once(init_d3d, [&] {
        __this->GetDevice(__uuidof(pDevice), reinterpret_cast<void**>(&pDevice));
        pDevice->GetImmediateContext(&pContext);
    });
	if (!State.ImGuiInitialized) {
        if (ImGuiInitialization(__this)) {
            State.ImGuiInitialized = true;
        } else {
            ReleaseSemaphore(DirectX::hRenderSemaphore, 1, NULL);
            return oPresent(__this, SyncInterval, Flags);
        }
    }

    WaitForSingleObject(DirectX::hRenderSemaphore, INFINITE);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ApplyTheme();

    if (State.ShowMenu)
    {
        ImGuiRenderer::Submit([]() { Menu::Render(); });
    }

    // Render in a separate thread
	std::async(std::launch::async, ImGuiRenderer::ExecuteQueue);

    ImGui::EndFrame();
    ImGui::Render();

    pContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT result = oPresent(__this, SyncInterval, Flags);

    ReleaseSemaphore(DirectX::hRenderSemaphore, 1, NULL);

    return result;
}

void DirectX::Shutdown() {
    assert(hRenderSemaphore != NULL); //Initialization is now in a hook, so we might as well guard against this
    for (uint8_t i = 0; i < MAX_RENDER_THREAD_COUNT; i++) //This ugly little hack means we use up all the render queues so we can end everything
    {
        assert(WaitForSingleObject(hRenderSemaphore, INFINITE) == WAIT_OBJECT_0); //Since this is only used on debug builds, we'll leave this for now
    }
    oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CloseHandle(hRenderSemaphore);
}
