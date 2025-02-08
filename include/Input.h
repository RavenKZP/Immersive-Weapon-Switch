#pragma once
#include "imgui.h"

namespace Input
{
	enum class DEVICE
	{
		kKeyboard,
		kMouse,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	class Manager final :
		public clib_util::singleton::ISingleton<Manager>
	{
	public:

		DEVICE GetInputDevice() const;
		void UpdateInputDevice(RE::InputEvent* event);
		uint32_t Convert(uint32_t button_key) const;

	private:
		static ImGuiKey ToImGuiKey(KEY a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_DIRECTX a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_ORBIS a_key);
		void            SendKeyEvent(std::uint32_t a_key, float a_value, bool a_keyPressed) const;

		// members
		bool screenshotQueued{ false };
		bool menuHidden{ false };

		float keyHeldDuration{ 0.5 };

		std::uint32_t screenshotKeyboard{ 0 };
		std::uint32_t screenshotMouse{ 0 };
		std::uint32_t screenshotGamepad{ 0 };

		DEVICE inputDevice{ DEVICE::kKeyboard };
	};
}
