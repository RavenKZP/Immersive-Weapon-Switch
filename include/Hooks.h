#pragma once

namespace Hooks {

    void Install();

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

    struct RemoveItemHook {
        using Fn = RE::ObjectRefHandle*(RE::TESObjectREFR*, RE::ObjectRefHandle&, RE::TESBoundObject*, std::int32_t,
                                        RE::ITEM_REMOVE_REASON, RE::ExtraDataList*, RE::TESObjectREFR*,
                                        const RE::NiPoint3*, const RE::NiPoint3*);

        static RE::ObjectRefHandle* thunk(RE::TESObjectREFR* a_this, RE::ObjectRefHandle& a_hidden_return_argument,
                                          RE::TESBoundObject* a_item, std::int32_t a_count,
                                          RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extra_list,
                                          RE::TESObjectREFR* a_move_to_ref, const RE::NiPoint3* a_drop_loc,
                                          const RE::NiPoint3* a_rotate);
        static void InstallHook();
        static inline Fn* func;

        static inline RE::TESObjectREFR* remove_act;
        static inline RE::TESBoundObject* remove_obj;
    };

    struct InputHook {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event);
        static inline REL::Relocation<decltype(thunk)> func;
        static void ProcessInput(RE::InputEvent* event);
    };
}
