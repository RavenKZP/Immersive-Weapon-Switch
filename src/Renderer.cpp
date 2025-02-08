#include "Renderer.h"

#include "Hooks.h"
#include "IconsFonts.h"
#include "IconsFontAwesome6.h"
#include "MCP.h"
#include "Manager.h"
#include "Settings.h"
#include "Styles.h"
#include "Utils.h"

#include <codecvt>
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

using namespace ImGui::Renderer;

LRESULT WndProc::thunk(const HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam) {
    auto& io = ImGui::GetIO();
    if (uMsg == WM_KILLFOCUS) {
        io.ClearInputKeys();
    }
    return func(hWnd, uMsg, wParam, lParam);
}

void CreateD3DAndSwapChain::thunk() {
    func();

    if (const auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
        swapChain = reinterpret_cast<IDXGISwapChain*>(renderer->data.renderWindows[0].swapChain);
        if (!swapChain) {
            logger::error("couldn't find swapChain");
            return;
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapChain->GetDesc(std::addressof(desc)))) {
            logger::error("IDXGISwapChain::GetDesc failed.");
            return;
        }

        device = reinterpret_cast<ID3D11Device*>(renderer->data.forwarder);
        context = reinterpret_cast<ID3D11DeviceContext*>(renderer->data.context);

        logger::info("Initializing ImGui...");

        ImGui::CreateContext();

        auto& io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
        io.IniFilename = nullptr;

        if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
            logger::error("ImGui initialization failed (Win32)");
            return;
        }
        if (!ImGui_ImplDX11_Init(device, context)) {
            logger::error("ImGui initialization failed (DX11)");
            return;
        }

        MANAGER(IconFont)->LoadIcons();
        logger::info("ImGui initialized.");

        WndProc::func = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrA(desc.OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc::thunk)));
        if (!WndProc::func) {
            logger::error("SetWindowLongPtrA failed!");
        }
    }
}

void DrawHook::thunk(std::uint32_t a_timer) {
    func(a_timer);

    if (!Utils::ShowSwitchIcons) {
        return;
    }

    ImGui::Styles::GetSingleton()->OnStyleRefresh();


    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    {
        // trick imgui into rendering at game's real resolution (ie. if upscaled with Display Tweaks)
        static const auto screenSize = RE::BSGraphics::Renderer::GetScreenSize();

        auto& io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(screenSize.width);
        io.DisplaySize.y = static_cast<float>(screenSize.height);

        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui_ImplDX11_CreateDeviceObjects();
    }
    ImGui::NewFrame();
    { 
        Render();
    }
    ImGui::EndFrame();

    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGui::Renderer::Install() {
    auto& trampoline = SKSE::GetTrampoline();
    constexpr size_t size_per_hook = 14;
    trampoline.create(size_per_hook * 2);

    const REL::Relocation<std::uintptr_t> target{REL::RelocationID(75595, 77226)};  // BSGraphics::InitD3D
    CreateD3DAndSwapChain::func =
        trampoline.write_call<5>(target.address() + REL::Relocate(0x9, 0x275), CreateD3DAndSwapChain::thunk);
    const REL::Relocation<std::uintptr_t> target2{REL::RelocationID(75461, 77246)};  // BSGraphics::Renderer::End
    DrawHook::func = trampoline.write_call<5>(target2.address() + 0x9, DrawHook::thunk);

    logger::info("ImGui Hooks Installed");
}

float ImGui::Renderer::GetResolutionScale()
{
    static auto height = RE::BSGraphics::Renderer::GetScreenSize().height;
	return DisplayTweaks::borderlessUpscale ? DisplayTweaks::resolutionScale : static_cast<float>(height)/ 1080.0f;
}

inline std::string UnicodeToUtf8(unsigned int codepoint) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::string u8str = converter.to_bytes(codepoint);
    return u8str;
}

void ImGui::Renderer::Render() {

    // Get the screen size
    const auto [width, height] = RE::BSGraphics::Renderer::GetScreenSize();

    // Calculate position
    auto Settings = Settings::GetSingleton();
    const ImVec2 Pos(width * Settings->PosX, height * Settings->PosY);
    // Set the window position
    ImGui::SetNextWindowPos(Pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f)); // Padding for cleaner layout

    ImGui::Begin("WeaponSwitch", nullptr,
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground );
    /*
    if (Utils::NewObj1 != "") {
        std::string DisplayText = Utils::NewObj1 + ICON_FA_REPEAT + Utils::OldObj1;
        ImGui::Text(DisplayText.c_str());
    }
    if (Utils::NewObj2 != "") {
        std::string DisplayText = Utils::NewObj2 + ICON_FA_REPEAT + Utils::OldObj2;
        ImGui::Text(DisplayText.c_str());
    }
    */
    ImGui::Text(ICON_FA_REPEAT);

    ImGui::End();

    ImGui::PopStyleVar(2);
}

