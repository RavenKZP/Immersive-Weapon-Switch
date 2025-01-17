#pragma once


namespace Hooks {

	void Install();

	inline constexpr float held_threshold = 0.24f;

	struct ReadyWeaponHandlerHook {
        static void InstallHook();
	    static void thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, [[maybe_unused]] RE::PlayerControlsData* a_data);
		static inline REL::Relocation<decltype(thunk)> func;

	};

	struct EquipObjectHook {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                          RE::ExtraDataList* a_extraData = nullptr, std::uint32_t a_count = 1,
                          const RE::BGSEquipSlot* a_slot = nullptr, bool a_queueEquip = true, bool a_forceEquip = false,
                          bool a_playSounds = true, bool a_applyNow = false);
        static inline REL::Relocation<decltype(thunk)> func;
    };

	struct UnEquipObjectHook {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                          RE::ExtraDataList* a_extraData = nullptr, std::uint32_t a_count = 1,
                          const RE::BGSEquipSlot* a_slot = nullptr, bool a_queueEquip = true, bool a_forceEquip = false,
                          bool a_playSounds = true, bool a_applyNow = false,
                          const RE::BGSEquipSlot* a_slotToReplace = nullptr);
		static inline REL::Relocation<decltype(thunk)> func;
	};

	template <class T>
	struct ActorUpdateHook {
		static void InstallHook();
		static void thunk(T* a_actor, float a_delta);
		static inline REL::Relocation<decltype(thunk)> func;
	};

}