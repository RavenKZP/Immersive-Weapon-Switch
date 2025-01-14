#pragma once


namespace Hooks {

	void Install();

	inline constexpr float held_threshold = 0.24f;

	struct ReadyWeaponHandlerHook {
        static void InstallHook();
	    static void thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, [[maybe_unused]] RE::PlayerControlsData* a_data);
		static inline REL::Relocation<decltype(thunk)> func;

	};

	struct EquipObjectOverRide {
        static void InstallHook();
        static void thunk(RE::ActorEquipManager* a_self, RE::Actor* a_actor, RE::TESBoundObject* a_object, std::uint64_t a_unk);
        static inline REL::Relocation<decltype(thunk)> func;
    };
}