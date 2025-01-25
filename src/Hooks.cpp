#include "Hooks.h"
#include "Utils.h"

namespace Hooks {

    void Install() {
        constexpr size_t size_per_hook = 14;
        auto& trampoline = SKSE::GetTrampoline();
        SKSE::AllocTrampoline(size_per_hook * 4);

        ReadyWeaponHandlerHook::InstallHook();
        EquipObjectHook::InstallHook(trampoline);
        EquipObjectUoTHook::InstallHook(trampoline);
        EquipSpellHook::InstallHook(trampoline);
        UnEquipObjectHook::InstallHook(trampoline);
        ActorUpdateHook<RE::Character>::InstallHook();
        ActorUpdateHook<RE::PlayerCharacter>::InstallHook();
    }

    void ReadyWeaponHandlerHook::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_ReadyWeaponHandler[0]);
        func = vTable.write_vfunc(0x4, &ReadyWeaponHandlerHook::thunk);
    }
    void EquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        //PC
        func = a_trampoline.write_call<5>(REL::RelocationID(37951, 38907).address() + REL::Relocate(0x2e0, 0x2e0), thunk);
        //NPC:
        func = a_trampoline.write_call<5>(REL::RelocationID(46955, 48124).address() + REL::Relocate(0x1a5, 0x1d6), thunk);
        //Brawl
        func = a_trampoline.write_call<5>(REL::RelocationID(53861, 54661).address() + REL::Relocate(0x14e, 0x14e), thunk);
    }
    void EquipObjectUoTHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // Use or Take
        func = a_trampoline.write_call<5>(REL::RelocationID(37938, 38894).address() + REL::Relocate(0xE5, 0x170), thunk);
    }
    void EquipSpellHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        func = a_trampoline.write_call<5>(REL::RelocationID(37952, 38908).address() + REL::Relocate(0xd7, 0xd7), thunk);
    }
    void UnEquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        //PC
        func = a_trampoline.write_call<5>(REL::RelocationID(37951, 38907).address() + REL::Relocate(0x2a9, 0x2a9), thunk);
        //NPC:
        func = a_trampoline.write_call<5>(REL::RelocationID(38789, 39814).address() + REL::Relocate(0x124, 0x11e), thunk);
    }
    template <class T>
    void ActorUpdateHook<T>::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(T::VTABLE[0]);
        func = vTable.write_vfunc(0xAD, &ActorUpdateHook::thunk);
    }

    void ReadyWeaponHandlerHook::thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event,
                                       RE::PlayerControlsData* a_data) {
        if (!a_this || !a_event || !a_data) {
            return func(a_this, a_event, a_data);
        }

        RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
        if (!player->AsActorState()->IsWeaponDrawn()) {
            return func(a_this, a_event, a_data);
        }

        if (!a_event->IsUp()) return;
        if (a_event->HeldDuration() < held_threshold) {
            a_event->value = 1.0f;
            a_event->heldDownSecs = 0.f;
            return func(a_this, a_event, a_data);
        }

        RE::TESForm* weapon_L = player->GetEquippedObject(true);
        RE::TESForm* weapon_or_shield = weapon_L ? weapon_L : player->GetEquippedObject(false);
        if (!weapon_or_shield) {
            return func(a_this, a_event, a_data);
        }
        if (weapon_or_shield->IsWeapon() || weapon_or_shield->IsArmor()) {
            SKSE::GetTaskInterface()->AddTask([player, weapon_or_shield]() {
                RE::TESBoundObject* bound = weapon_or_shield->As<RE::TESBoundObject>();
                player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
            });
        }

        return func(a_this, a_event, a_data);
    }

    void EquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                RE::ExtraDataList* a_extraData, std::uint32_t a_count, const RE::BGSEquipSlot* a_slot,
                                bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow) {
        if (!a_actor || !a_manager) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow);
        }
        logger::trace("Equip Hook");
        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->GetFormID())) {
                logger::trace("{} is in Queue not allowing to Equip {}, Adding to Queue", a_actor->GetName(),
                              a_object ? a_object->GetName() : "Nothing");
                const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                             a_queueEquip, a_forceEquip, a_playSounds, a_applyNow};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                return;
            }
            if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {

                    if (a_slot) {
                        if (Utils::IsHandFree(a_slot->GetFormID(), a_actor)) {
                            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip,
                                        a_forceEquip, a_playSounds, a_applyNow);
                        }
                    }

                    logger::trace("{} is in kDrawn Weapon State, adding to Queue with {}", a_actor->GetName(),
                                  a_object ? a_object->GetName() : "Nothing");

                    const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                                 a_queueEquip, a_forceEquip, a_playSounds, a_applyNow};
                    UpdateQueue(a_actor->GetFormID(), EquipEvent);
                    a_actor->DrawWeaponMagicHands(false);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    return;
                }
            }
        }

        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow);
    }

    void EquipObjectUoTHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object, std::uint64_t a_unk) {
        if (!a_actor || !a_object || !a_manager) {
            return func(a_manager, a_actor, a_object, a_unk);
        }
        logger::trace("Equip UoT Hook");
        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->GetFormID())) {
                logger::trace("{} is in Queue not allowing to Equip {}, Adding to Queue", a_actor->GetName(),
                              a_object ? a_object->GetName() : "Nothing");
                const Utils::EquipEvent EquipEvent{a_object};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                return;
            }
            if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    if (Utils::IsHandFree(Utils::EquipSlots::RightHand, a_actor)) {
                        return func(a_manager, a_actor, a_object, a_unk);
                    }

                    logger::trace("{} is in kDrawn Weapon State, adding to Queue with {}", a_actor->GetName(),
                                  a_object ? a_object->GetName() : "Nothing");

                    const Utils::EquipEvent EquipEvent{a_object};
                    UpdateQueue(a_actor->GetFormID(), EquipEvent);
                    a_actor->DrawWeaponMagicHands(false);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    return;
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_unk);
    }

    void EquipSpellHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::SpellItem* a_spell, RE::BGSEquipSlot** a_slot) {
        if (!a_spell || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_spell, a_slot);
        }
        logger::trace("Equip Spell Hook");

        if (Utils::IsInQueue(a_actor->GetFormID())) {
            logger::trace("{} is in Queue not allowing to Equip {}, Adding to Queue", a_actor->GetName(), a_spell->GetName());

            const Utils::EquipEvent EquipEvent{nullptr, nullptr, 1, *a_slot, true, false, true, false, true, a_spell};
            UpdateQueue(a_actor->GetFormID(), EquipEvent);
            return;
        }
        if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
            if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {

                if (a_slot) {
                    const RE::BGSEquipSlot* slot = *a_slot;
                    if (slot) {
                        if (Utils::IsHandFree(slot->GetFormID(), a_actor)) {
                            return func(a_manager, a_actor, a_spell, a_slot);
                        }
                    }
                }

                logger::trace("{} is in kDrawn Weapon State, adding to Queue with {}", a_actor->GetName(),
                              a_spell->GetName());

                const Utils::EquipEvent EquipEvent{nullptr, nullptr, 1, *a_slot, true, false, true, false, true, a_spell};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
        }
        return func(a_manager, a_actor, a_spell, a_slot);
    }

    void UnEquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                  RE::ExtraDataList* a_extraData, std::uint32_t a_count, const RE::BGSEquipSlot* a_slot,
                                  bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow,
                                  const RE::BGSEquipSlot* a_slotToReplace)
    {
        if (!a_object || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow, a_slotToReplace);
        }
        logger::trace("Unequip Hook");

        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->GetFormID())) {
                logger::trace("{} is in Queue not allowing to Unequip {}, Adding to Queue", a_actor->GetName(),
                              a_object->GetName());
                const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                             a_queueEquip, a_forceEquip, a_playSounds, a_applyNow, false};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                return;
            }

            if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    logger::trace("{} is in kDrawn Weapon State, adding to Queue with {}", a_actor->GetName(),
                                  a_object->GetName());
                    const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,    a_slot, a_queueEquip,
                                                       a_forceEquip, a_playSounds, a_applyNow, false};
                    UpdateQueue(a_actor->GetFormID(), EquipEvent);
                    a_actor->DrawWeaponMagicHands(false);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    return;
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow, a_slotToReplace);
    }

    template <class T>
    void ActorUpdateHook<T>::thunk(T* a_actor, float a_delta) {
        if (!a_actor) {
            return func(a_actor, a_delta);
        }
        if (!Utils::IsInQueue(a_actor->GetFormID())) {
            return func(a_actor, a_delta);
        }

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
            RE::WEAPON_STATE weaponState = actorState->GetWeaponState();

            if (weaponState != RE::WEAPON_STATE::kSheathed) {
                return func(a_actor, a_delta);
            }
            logger::trace("{} Can now perform all actions from, Queue", a_actor->GetName());
            std::queue<Utils::EquipEvent> equipQueue = Utils::GetQueue(a_actor->GetFormID());
            Utils::RemoveFromQueue(a_actor->GetFormID());
            while (!equipQueue.empty()) {
                if (const Utils::EquipEvent& currEvent = equipQueue.front(); currEvent.equip) {
                    if (currEvent.spell) {
                        logger::trace("Equipping spell: {}", currEvent.spell->GetName());
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(a_actor, currEvent.spell, currEvent.slot);
                    } else {
                        logger::trace("Equipping: {}", currEvent.object->GetName());
                        RE::ActorEquipManager::GetSingleton()->EquipObject(
                            a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                            currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                    }
                } else {
                    logger::trace("Unequipping: {}", currEvent.object->GetName());
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(
                        a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                        currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                }
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;
                equipQueue.pop();
            }
            a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToDraw;
            a_actor->DrawWeaponMagicHands(true);
        }
        return func(a_actor, a_delta);
    }

};
