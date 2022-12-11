#include <d3d11.h>
#include <d3dx11.h>
#include <xnamath.h>
#include <d3dcompiler.h>
#include "stdio.h"

#include "Hook.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment(lib, "d3dcompiler.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(WINAPI* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

HWND window;
WNDPROC oWndProc;
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

Present oPresent;
ID3D11Device* pDevice;
ID3D11DeviceContext* pDeviceContext;
ID3D11RenderTargetView* pRenderTargetView;

void CleanupRenderTargetView() {
    if (pRenderTargetView) {
        pRenderTargetView->Release();
        pRenderTargetView = nullptr;
    }
}

ResizeBuffers oResizeBuffers;
HRESULT WINAPI hResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height,DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    CleanupRenderTargetView();
    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

void CreateRenderTargetView(IDXGISwapChain* pSwapChain) {
    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    window = sd.OutputWindow;
    ID3D11Texture2D* pBackBuffer{};
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    pBackBuffer->Release();
}

bool init = false;
HRESULT WINAPI hPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!init)
    {
        pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
        pDevice->GetImmediateContext(&pDeviceContext);
        CreateRenderTargetView(pSwapChain);
        oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX11_Init(pDevice, pDeviceContext);
        init = true;
    }

    if (!pRenderTargetView) {
        CreateRenderTargetView(pSwapChain);
    }

    if (ImGui::GetCurrentContext() && pRenderTargetView) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("ImGui Window");
        ImGui::End();

        ImGui::Render();

        pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    return oPresent(pSwapChain, SyncInterval, Flags);
}

void CleanupImGui() {
    if (ImGui::GetCurrentContext()) {
        if (ImGui::GetIO().BackendRendererUserData)
            ImGui_ImplDX11_Shutdown();

        if (ImGui::GetIO().BackendPlatformUserData)
            ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();
    }
}

void CleanupDirect() {
    pDevice->Release();
    pRenderTargetView = nullptr;

    pDeviceContext->Release();
    pRenderTargetView = nullptr;
}

void Unload() {
    Hook::UnhookAll();

    CleanupImGui();
    CleanupRenderTargetView();
    CleanupDirect();

    SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));
}

void GetSwapChainVMT(void* vmt, size_t sizeVmt) {
    IDXGISwapChain* pFakeSwapChain{};

    DXGI_SWAP_CHAIN_DESC spawChainParam{};

    ZeroMemory(&spawChainParam, sizeof(DXGI_SWAP_CHAIN_DESC));

    spawChainParam.BufferCount = 1;
    spawChainParam.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    spawChainParam.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    spawChainParam.OutputWindow = GetDesktopWindow();
    spawChainParam.Windowed = TRUE;
    spawChainParam.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    spawChainParam.SampleDesc.Count = 1;

    HRESULT status = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &spawChainParam, &pFakeSwapChain, NULL, NULL, NULL);

    memcpy(vmt, *(void**)pFakeSwapChain, sizeVmt);

    pFakeSwapChain->Release();
}

void WINAPI Main(HMODULE hModule) {
    void* vmt[128];
    GetSwapChainVMT(vmt, sizeof(vmt));
    oPresent = (Present)vmt[8];
    oResizeBuffers = (ResizeBuffers)vmt[13];
    Hook::HookFunction(hPresent, (void**)&oPresent, 5);
    Hook::HookFunction(hResizeBuffers, (void**)&oResizeBuffers, 5);
#ifdef _DEBUG
    FILE* file;
    AllocConsole();
    freopen_s(&file, "CONOUT$", "w", stdout);
    printf("Hello world");

    while (true) {
        Sleep(50);
        if (GetAsyncKeyState(VK_END) & 1)
            break;
    }

    fclose(file);
    FreeConsole();
    Unload();
    FreeLibraryAndExitThread(static_cast<HMODULE>(hModule), 0);
#endif // _DEBUG
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Main, hModule, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        Unload();
        break;
    }
    return TRUE;
}

