#pragma once


namespace Hooks {

	void Install();

	inline constexpr float held_threshold = 0.24f;

	struct ReadyWeaponHandlerHook {
        static void InstallHook();
	    static void thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, [[maybe_unused]] RE::PlayerControlsData* a_data);
		static inline REL::Relocation<decltype(thunk)> func;

	};
}