#pragma once

namespace Hooks {

    void Install();
    void InstallReadOnly();

    struct EquipObjectHook {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                          std::uint64_t a_unk);
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct EquipSpellHook {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::SpellItem* a_spell,
                          RE::BGSEquipSlot** a_slot);
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct UnEquipObjectHook {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                          std::uint64_t a_unk);
        static inline REL::Relocation<decltype(thunk)> func;
    };

    template <class T>
    struct RemoveItemHook {
        static void InstallHook();
        static RE::ObjectRefHandle* thunk(T* a_this, RE::ObjectRefHandle& a_hidden_return_argument,
                                          RE::TESBoundObject* a_item, std::int32_t a_count,
                                          RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extra_list,
                                          RE::TESObjectREFR* a_move_to_ref, const RE::NiPoint3* a_drop_loc,
                                          const RE::NiPoint3* a_rotate);
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct InputHook {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event);
        static inline REL::Relocation<decltype(thunk)> func;
        static void ProcessInput(RE::InputEvent* event);
    };
}
