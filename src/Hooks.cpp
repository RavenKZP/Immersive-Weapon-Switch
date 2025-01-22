#include "Hooks.h"
#include "Utils.h"


// ToDo and Ideas
// 
// General/New hooks?
// ToDo: Inventory showing what item will be quiped (like it's equiped now)
//
// When PC start combat (in town), and change weapon NPC stop combat
// Because NPC stop combat when PC is Sheatling a weapon
//
// ReadyWeaponHandlerHook
// ToDo: Add some force to object (like in Grab an throw)
// ToDo: Call attack unarmed Letf or Right hand (Or some vanilla animation tah can fit)


namespace Hooks {

    void Install() {
        constexpr size_t size_per_hook = 14;
        auto& trampoline = SKSE::GetTrampoline();
        SKSE::AllocTrampoline(size_per_hook * 3);

        ReadyWeaponHandlerHook::InstallHook();
        EquipObjectHook::InstallHook(trampoline);
        EquipSpellHook::InstallHook(trampoline);
        UnEquipObjectHook::InstallHook(trampoline);
        ActorUpdateHook<RE::Character>::InstallHook();
        ActorUpdateHook<RE::PlayerCharacter>::InstallHook();
    }

    void ReadyWeaponHandlerHook::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_ReadyWeaponHandler[0]);
        func = vTable.write_vfunc(0x4, &ReadyWeaponHandlerHook::thunk);
    }
    void EquipObjectHook::InstallHook(SKSE::Trampoline& trampoline) {
        //PC
        EquipObjectHook::func = trampoline.write_call<5>(REL::RelocationID(37951, 38907).address() + REL::Relocate(0x2e0, 0x2e0), thunk);
        //NPC:
        EquipObjectHook::func = trampoline.write_call<5>(REL::RelocationID(46955, 48124).address() + REL::Relocate(0x1a5, 0x1d6), thunk);

    }
    void EquipSpellHook::InstallHook(SKSE::Trampoline& trampoline) {
        EquipSpellHook::func = trampoline.write_call<5>(REL::RelocationID(37952, 38908).address() + REL::Relocate(0xd7, 0xd7), thunk);
    }
    void UnEquipObjectHook::InstallHook(SKSE::Trampoline& trampoline) {
        //PC
        UnEquipObjectHook::func = trampoline.write_call<5>(REL::RelocationID(37951, 38907).address() + REL::Relocate(0x2a9, 0x2a9), thunk);
        //NPC:
        UnEquipObjectHook::func = trampoline.write_call<5>(REL::RelocationID(38789, 39814).address() + REL::Relocate(0x124, 0x11e), thunk);
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
                //RE::ActorEquipManager::GetSingleton()->UnequipObject(player, bound);
                player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
            });
        }

        return func(a_this, a_event, a_data);
    }

    void EquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                RE::ExtraDataList* a_extraData, std::uint32_t a_count, const RE::BGSEquipSlot* a_slot,
                                bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow) {
        if (!a_object || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow);
        }

        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->formID)) {
                logger::trace("{} is in Queue not allowing to Equip {}", a_actor->GetName(), a_object->GetName());
                Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                             a_queueEquip, a_forceEquip, a_playSounds, a_applyNow};
                Utils::UpdateQueue(a_actor->formID, EquipEvent);
                return;
            }
            if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {

                    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
                    RE::TESForm* RightHandObj = player->GetEquippedObject(false);
                    RE::TESForm* LeftHandObj = player->GetEquippedObject(true);

                    if (a_slot) {
                        if (((a_slot->formID == 81731) && (LeftHandObj == nullptr)) || 
                            ((a_slot->formID == 82408) && (LeftHandObj == nullptr)) ||
                            ((a_slot->formID == 81731) && (LeftHandObj->Is(RE::FormType::Spell))) ||
                            ((a_slot->formID == 81730) && (RightHandObj->Is(RE::FormType::Spell))) || 
                            ((a_slot->formID == 81730) && (RightHandObj == nullptr))) {
                            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip,
                                        a_forceEquip, a_playSounds, a_applyNow);
                        }
                    }

                    logger::trace("{} is in kDrawn Weapon State, adding to Queue with {}", a_actor->GetName(),
                                  a_object->GetName());

                    Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                                 a_queueEquip, a_forceEquip, a_playSounds, a_applyNow};
                    Utils::UpdateQueue(a_actor->formID, EquipEvent);
                    a_actor->DrawWeaponMagicHands(false);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    return;
                }
            }
        }

        logger::trace("call func");
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow);
    }

    void EquipSpellHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::SpellItem* a_spell, RE::BGSEquipSlot** a_slot) {
        if (!a_slot || !a_spell || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_spell, a_slot);
        }

        if (Utils::IsInQueue(a_actor->formID)) {
            logger::trace("{} is in Queue not allowing to Equip {}", a_actor->GetName(), a_spell->GetName());

            Utils::EquipEvent EquipEvent{nullptr, nullptr, 1, *a_slot, true, false, true, false, true, a_spell};
            Utils::UpdateQueue(a_actor->formID, EquipEvent);
            return;
        }
        if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
            if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {

                RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
                RE::TESForm* RightHandObj = player->GetEquippedObject(false);
                RE::TESForm* LeftHandObj = player->GetEquippedObject(true);

                if (a_slot) {
                    RE::BGSEquipSlot* slot = *a_slot;
                    if (((slot->formID == 81731) && (LeftHandObj == nullptr)) ||
                        ((slot->formID == 81730) && (RightHandObj == nullptr)) ||
                        ((slot->formID == 81731) && (LeftHandObj->Is(RE::FormType::Spell))) ||
                        ((slot->formID == 81730) && (RightHandObj->Is(RE::FormType::Spell)))) {
                        return func(a_manager, a_actor, a_spell, a_slot);
                    }
                }

                logger::trace("{} is in kDrawn Weapon State, adding to Queue with {}", a_actor->GetName(),
                              a_spell->GetName());

                Utils::EquipEvent EquipEvent{nullptr, nullptr, 1, *a_slot, true, false, true, false, true, a_spell};
                Utils::UpdateQueue(a_actor->formID, EquipEvent);
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

        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->formID)) {
                logger::trace("{} is in Queue not allowing to Unequip {}", a_actor->GetName(),
                              a_object->GetName());
                Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                             a_queueEquip, a_forceEquip, a_playSounds, a_applyNow, false};
                Utils::UpdateQueue(a_actor->formID, EquipEvent);
                return;
            }

            logger::trace("{} is NOT IsInPospondQueue", a_actor->GetName());
            if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    logger::trace("{} is in kDrawn Weapon State, adding to Queue with {}", a_actor->GetName(),
                                  a_object->GetName());
                    Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,    a_slot, a_queueEquip,
                                                 a_forceEquip, a_playSounds, a_applyNow, false};
                    Utils::UpdateQueue(a_actor->formID, EquipEvent);
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
        if (!Utils::IsInQueue(a_actor->formID)) {
            return func(a_actor, a_delta);
        }        

        if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
            if (actorState->GetWeaponState() != RE::WEAPON_STATE::kSheathed) {
                return func(a_actor, a_delta);
            } else {
                logger::trace("{} Can now perform all actions from, Queue", a_actor->GetName());
                std::queue<Utils::EquipEvent> equipQueue = Utils::GetQueue(a_actor->formID);
                while (!equipQueue.empty()) {
                    const Utils::EquipEvent& currEvent = equipQueue.front();

                    if (currEvent.equip) {
                        if (currEvent.spell) {
                            if (!currEvent.slot)
                                logger::trace("slot is nullptr <PANIC>");
                            logger::trace("{} Equiping spell: {} to slot nr {}", a_actor->GetName(), currEvent.spell->GetName(),
                                          currEvent.slot->formID);
                            RE::ActorEquipManager::GetSingleton()->EquipSpell(a_actor, currEvent.spell, currEvent.slot);
                        } else {
                            logger::trace("Equiping: {}", currEvent.object->GetName());
                            RE::ActorEquipManager::GetSingleton()->EquipObject(
                                a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                                currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                        }
                    } else {
                        logger::trace("Unequiping: {}", currEvent.object->GetName());
                        RE::ActorEquipManager::GetSingleton()->UnequipObject(
                            a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                            currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                    }
                    equipQueue.pop();
                    a_actor->DrawWeaponMagicHands(true);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToDraw;
                }
                Utils::RemoveFromQueue(a_actor->formID);
            }
          
        }
        return func(a_actor, a_delta);
    }

};
