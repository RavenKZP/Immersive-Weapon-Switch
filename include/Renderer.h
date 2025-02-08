#pragma once
#include <atomic>
#include <shared_mutex>

#include "MCP.h"
#include "Settings.h"

namespace ImGui::Renderer
{

    void Install();

    struct WndProc {
        static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static inline WNDPROC func;
    };

    struct CreateD3DAndSwapChain {
        static inline IDXGISwapChain* swapChain = nullptr;
        static inline ID3D11Device* device = nullptr;
        static inline ID3D11DeviceContext* context = nullptr;
        static void thunk();
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct DrawHook {
        static void thunk(std::uint32_t a_timer);
        static inline REL::Relocation<decltype(thunk)> func;
    };

	namespace DisplayTweaks
	{
		inline float resolutionScale{ 1.0f };
		inline bool  borderlessUpscale{ false };
	}

    inline void LoadSettings(const CSimpleIniA& a_ini)
	{
		DisplayTweaks::resolutionScale = static_cast<float>(a_ini.GetDoubleValue("Render", "ResolutionScale", DisplayTweaks::resolutionScale));
		DisplayTweaks::borderlessUpscale = static_cast<float>(a_ini.GetBoolValue("Render", "BorderlessUpscale", DisplayTweaks::borderlessUpscale));
	}

	float GetResolutionScale();
	void Render(); // starts here
	
}