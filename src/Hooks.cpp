#include "Events.h"
#include "Hooks.h"
#include "Settings.h"
#include "Utils.h"

namespace Hooks {

    void Install() {
        constexpr size_t size_per_hook = 14;
        auto& trampoline = SKSE::GetTrampoline();
        SKSE::AllocTrampoline(size_per_hook * 5);

        ReadyWeaponHandlerHook::InstallHook();

        EquipObjectHook::InstallHook(trampoline);
        EquipObjectNoSlotHook::InstallHook(trampoline);
        EquipSpellHook::InstallHook(trampoline);
        UnEquipObjectPCHook::InstallHook(trampoline);
        UnEquipObjectNPCHook::InstallHook(trampoline);

        ActorUpdateHook<RE::Character>::InstallHook();
        ActorUpdateHook<RE::PlayerCharacter>::InstallHook();

        logger::info("Hooks Installed");
    }

    void ReadyWeaponHandlerHook::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_ReadyWeaponHandler[0]);
        func = vTable.write_vfunc(0x4, &ReadyWeaponHandlerHook::thunk);
    }
    void EquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // PC
        func =
            a_trampoline.write_call<5>(REL::RelocationID(37951, 38907).address() + REL::Relocate(0x2e0, 0x2e0), thunk);
        // NPC
        func =
            a_trampoline.write_call<5>(REL::RelocationID(46955, 48124).address() + REL::Relocate(0x1a5, 0x1d6), thunk);
        // Brawl (Papyrus?)
        func =
            a_trampoline.write_call<5>(REL::RelocationID(53861, 54661).address() + REL::Relocate(0x14e, 0x14e), thunk);
    }
    void EquipObjectNoSlotHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // Use or Take 
        // This is general hook called very often, but it's lacking equip slot info
        func =
            a_trampoline.write_call<5>(REL::RelocationID(37938, 38894).address() + REL::Relocate(0xE5, 0x170), thunk);
    }
    void EquipSpellHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // PC and NPC
        func = a_trampoline.write_call<5>(REL::RelocationID(37952, 38908).address() + REL::Relocate(0xd7, 0xd7), thunk);
    }
    void UnEquipObjectPCHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // PC
        func =
            a_trampoline.write_call<5>(REL::RelocationID(37951, 38907).address() + REL::Relocate(0x2a9, 0x2a9), thunk);
    }
    void UnEquipObjectNPCHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // NPC
        // Called also on PC but in a very wierd conditions, so needed only for NPC
        func =
            a_trampoline.write_call<5>(REL::RelocationID(37949, 38905).address() + REL::Relocate(0xfd, 0x101), thunk);
    }
    template <class T>
    void ActorUpdateHook<T>::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(T::VTABLE[0]);
        func = vTable.write_vfunc(0xAD, &ActorUpdateHook::thunk);
    }

    void ReadyWeaponHandlerHook::thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event,
                                       RE::PlayerControlsData* a_data) {
        if (!a_this || !a_event || !a_data || !Settings::Mod_Active) {
            return func(a_this, a_event, a_data);
        }

        RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
        if (!player->AsActorState()->IsWeaponDrawn()) {
            return func(a_this, a_event, a_data);
        }

        if (!a_event->IsUp()) return;
        if (a_event->HeldDuration() < Settings::Held_Duration) {
            a_event->value = 1.0f;
            a_event->heldDownSecs = 0.f;
            return func(a_this, a_event, a_data);
        }

        if (Settings::Hold_To_Drop) {
            RE::TESForm* RightHand = player->GetEquippedObject(false);
            RE::TESForm* LeftHand = player->GetEquippedObject(true);

            if ((RightHand) && (RightHand->IsWeapon())) {
                RE::TESBoundObject* bound = RightHand->As<RE::TESBoundObject>();
                player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
            }
            if ((LeftHand) && (LeftHand->IsWeapon() || LeftHand->IsArmor() || LeftHand->Is(RE::FormType::Light))) {
                RE::TESBoundObject* bound = LeftHand->As<RE::TESBoundObject>();
                player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
            }
        }
        if (Settings::Hold_To_Unarmed) {
            RE::ActorEquipManager::GetSingleton()->EquipObject(player, Utils::unarmed_weapon, nullptr, 1,
                                                               Utils::right_hand_slot);
            RE::ActorEquipManager::GetSingleton()->EquipObject(player, Utils::unarmed_weapon, nullptr, 1,
                                                               Utils::left_hand_slot);
        }

        return func(a_this, a_event, a_data);
    }

    void EquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                RE::ExtraDataList* a_extraData, std::uint32_t a_count, const RE::BGSEquipSlot* a_slot,
                                bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow) {
        if (!a_actor || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow);
        }
        logger::trace("Equip Hook: {} {}", a_actor->GetName(), a_object->GetName());

        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->GetFormID())) {
                logger::trace("Equip Hook: {} Is In Queue", a_actor->GetName());
                const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                                   a_queueEquip, a_forceEquip, a_playSounds, a_applyNow};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                if (a_actor->IsPlayerRef()) {
                    auto keywordForm = a_object->As<RE::BGSKeywordForm>();
                    bool left = false;
                    if (a_slot) {
                        if ((a_slot->GetFormID() == Utils::EquipSlots::Left) ||
                            (a_slot->GetFormID() == Utils::EquipSlots::Shield)) {
                            left = true;
                        }
                    }
                    if ((a_object->IsArmor()) || (a_object->Is(RE::FormType::Light))) {
                        left = true;
                    }
                    Utils::SetInventoryInfo(keywordForm, left);
                    auto UnequipObj = a_actor->GetEquippedObject(left);
                    if (UnequipObj) {
                        auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                        Utils::SetInventoryInfo(uneqKwdForm, left, true);
                    }
                    if (Utils::IsTwoHanded(a_object)) {
                        UnequipObj = a_actor->GetEquippedObject(!left);
                        if (UnequipObj) {
                            auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                            Utils::SetInventoryInfo(uneqKwdForm, left, true);
                        }
                    }
                }
                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
            if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    if (a_slot) {
                        if (Utils::IsHandFree(a_slot->GetFormID(), a_actor, a_object)) {
                            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip,
                                        a_forceEquip, a_playSounds, a_applyNow);
                        }
                    }
                    if (Utils::DropIfLowHP(a_actor)) {
                        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip,
                                    a_forceEquip, a_playSounds, a_applyNow);
                    }

                    logger::trace("Equip Hook: Add {} To Queue", a_actor->GetName());

                    const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,      a_slot,
                                                       a_queueEquip, a_forceEquip, a_playSounds, a_applyNow};
                    UpdateQueue(a_actor->GetFormID(), EquipEvent);
                    //Events::CreateEventSink(a_actor);
                    if (a_actor->IsPlayerRef()) {
                        auto keywordForm = a_object->As<RE::BGSKeywordForm>();
                        bool left = false;
                        if (a_slot) { 
                            if ((a_slot->GetFormID() == Utils::EquipSlots::Left) ||
                                (a_slot->GetFormID() == Utils::EquipSlots::Shield)) {
                                left = true;
                            }
                        }
                        if ((a_object->IsArmor()) || (a_object->Is(RE::FormType::Light))) {
                            left = true;
                        }
                        Utils::SetInventoryInfo(keywordForm, left);
                        auto UnequipObj = a_actor->GetEquippedObject(left);
                        if (UnequipObj) {
                            auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                            Utils::SetInventoryInfo(uneqKwdForm, left, true);
                        }
                        if (Utils::IsTwoHanded(a_object)) {
                            UnequipObj = a_actor->GetEquippedObject(!left);
                            if (UnequipObj) {
                                auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                                Utils::SetInventoryInfo(uneqKwdForm, left, true);
                            }
                        }
                    }
                    a_actor->DrawWeaponMagicHands(false);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    return;
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow);
    }

    void EquipObjectNoSlotHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor,
                                      RE::TESBoundObject* a_object, std::uint64_t a_unk) {
        if (!a_actor || !a_object || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_unk);
        }
        logger::trace("Equip Hook No Slot: {} {}", a_actor->GetName(), a_object->GetName());

        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->GetFormID())) {
                logger::trace("Equip Hook No Slot: {} Is In Queue", a_actor->GetName());
                // Check if this object is already tracked by EquipHook
                // This function is always called After Main Equip hook 
                // but sometimes (e.g. Use or Take) call this dirlectly
                if (a_object == Utils::GetLastObject(a_actor->GetFormID())) {
                    return func(a_manager, a_actor, a_object, a_unk);
                }
                Utils::EquipEvent EquipEvent{a_object};
                Utils::EquipSlots slot = Utils::SetEquipSlot(a_object, a_actor);
                if (Utils::IsHandFree(slot, a_actor, a_object)) {
                    if (slot != Utils::EquipSlots::Left) {
                        return func(a_manager, a_actor, a_object, a_unk);
                    } else {
                        EquipEvent.slot = Utils::left_hand_slot;
                    }
                }
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                if (a_actor->IsPlayerRef()) {
                    auto keywordForm = a_object->As<RE::BGSKeywordForm>();
                    bool left = false;
                    if ((slot == Utils::EquipSlots::Left) || (slot == Utils::EquipSlots::Shield)) {
                        left = true;
                    }
                    if ((a_object->IsArmor()) || (a_object->Is(RE::FormType::Light))) {
                        left = true;
                    }
                    Utils::SetInventoryInfo(keywordForm, left);
                    auto UnequipObj = a_actor->GetEquippedObject(left);
                    if (UnequipObj) {
                        auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                        Utils::SetInventoryInfo(uneqKwdForm, left, true);
                    }
                    if (Utils::IsTwoHanded(a_object)) {
                        UnequipObj = a_actor->GetEquippedObject(!left);
                        if (UnequipObj) {
                            auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                            Utils::SetInventoryInfo(uneqKwdForm, left, true);
                        }
                    }
                }
                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
            if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    Utils::EquipEvent EquipEvent{a_object};
                    Utils::EquipSlots slot = Utils::SetEquipSlot(a_object, a_actor);
                    if (Utils::IsHandFree(slot, a_actor, a_object)) {
                        if (slot != Utils::EquipSlots::Left) {
                            return func(a_manager, a_actor, a_object, a_unk);
                        } else {
                            EquipEvent.slot = Utils::left_hand_slot;
                        }
                    }

                    logger::trace("Equip Hook No Slot: Add {} To Queue", a_actor->GetName());
                    UpdateQueue(a_actor->GetFormID(), EquipEvent);
                    //Events::CreateEventSink(a_actor);
                    if (a_actor->IsPlayerRef()) {
                        auto keywordForm = a_object->As<RE::BGSKeywordForm>();
                        bool left = false;
                        if ((slot == Utils::EquipSlots::Left) || (slot == Utils::EquipSlots::Shield)) {
                            left = true;
                        }
                        if ((a_object->IsArmor()) || (a_object->Is(RE::FormType::Light))) {
                            left = true;
                        }
                        Utils::SetInventoryInfo(keywordForm, left);
                        auto UnequipObj = a_actor->GetEquippedObject(left);
                        if (UnequipObj) {
                            auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                            Utils::SetInventoryInfo(uneqKwdForm, left, true);
                        }
                        if (Utils::IsTwoHanded(a_object)) {
                            UnequipObj = a_actor->GetEquippedObject(!left);
                            if (UnequipObj) {
                                auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                                Utils::SetInventoryInfo(uneqKwdForm, left, true);
                            }
                        }
                    }
                    a_actor->DrawWeaponMagicHands(false);
                    if ((slot == Utils::EquipSlots::Left) && Utils::IsHandFree(slot, a_actor, a_object)) {
                        // Little trick for instant equip
                        a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;
                    } else {
                        a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    }
                    return;
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_unk);
    }

    void EquipSpellHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::SpellItem* a_spell,
                               RE::BGSEquipSlot** a_slot) {
        if (!a_spell || !a_actor || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_spell, a_slot);
        }
        logger::trace("Equip Spell Hook: {} {}", a_actor->GetName(), a_spell->GetName());

        if (Utils::IsInQueue(a_actor->GetFormID())) {
            logger::trace("Equip Spell Hook: {} Is In Queue", a_actor->GetName());
            const Utils::EquipEvent EquipEvent{nullptr, nullptr, 1, *a_slot, true, false, true, false, true, a_spell};
            UpdateQueue(a_actor->GetFormID(), EquipEvent);
            if (a_actor->IsPlayerRef()) {
                auto keywordForm = a_spell->As<RE::BGSKeywordForm>();
                bool left = false;
                const RE::BGSEquipSlot* slot = *a_slot;
                if (slot->GetFormID() == Utils::EquipSlots::Left) {
                    left = true;
                }
                Utils::SetInventoryInfo(keywordForm, left);
                auto UnequipObj = a_actor->GetEquippedObject(left);
                if (UnequipObj) {
                    auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                    Utils::SetInventoryInfo(uneqKwdForm, left, true);
                }
            }
            a_actor->DrawWeaponMagicHands(false);
            a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
            return;
        }
        if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
            if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                if (a_slot) {
                    const RE::BGSEquipSlot* slot = *a_slot;
                    if (slot) {
                        if (Utils::IsHandFree(slot->GetFormID(), a_actor, nullptr)) {
                            return func(a_manager, a_actor, a_spell, a_slot);
                        }
                    }
                }

                if (Utils::DropIfLowHP(a_actor)) {
                    return func(a_manager, a_actor, a_spell, a_slot);
                }

                
                logger::trace("Equip Spell Hook: Add {} To Queue", a_actor->GetName());
                const Utils::EquipEvent EquipEvent{nullptr, nullptr, 1,     *a_slot, true,
                                                   false,   true,    false, true,    a_spell};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                //Events::CreateEventSink(a_actor);
                if (a_actor->IsPlayerRef()) {
                    auto keywordForm = a_spell->As<RE::BGSKeywordForm>();
                    bool left = false;
                    const RE::BGSEquipSlot* slot = *a_slot;
                    if (slot->GetFormID() == Utils::EquipSlots::Left) {
                        left = true;
                    }
                    Utils::SetInventoryInfo(keywordForm, left);
                    auto UnequipObj = a_actor->GetEquippedObject(left);
                    if (UnequipObj) {
                        auto uneqKwdForm = UnequipObj->As<RE::BGSKeywordForm>();
                        Utils::SetInventoryInfo(uneqKwdForm, left, true);
                    }
                }
                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
        }
        return func(a_manager, a_actor, a_spell, a_slot);
    }

    void UnEquipObjectPCHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                    RE::ExtraDataList* a_extraData, std::uint32_t a_count,
                                    const RE::BGSEquipSlot* a_slot, bool a_queueEquip, bool a_forceEquip,
                                    bool a_playSounds, bool a_applyNow, const RE::BGSEquipSlot* a_slotToReplace) {
        if (!a_object || !a_actor || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow, a_slotToReplace);
        }
        if (a_object->IsWeapon()) {
            if ((a_object == Utils::unarmed_weapon) || (a_object->As<RE::TESObjectWEAP>()->IsBound())) {
                return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                            a_playSounds, a_applyNow, a_slotToReplace);
            }
        }
        logger::trace("UnEquip Hook: {} {}", a_actor->GetName(), a_object->GetName());

        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->GetFormID())) {
                logger::trace("UnEquip PC Hook: {} Is In Queue", a_actor->GetName());
                const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,    a_slot, a_queueEquip,
                                                   a_forceEquip, a_playSounds, a_applyNow, false};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                if (a_actor->IsPlayerRef()) {
                    auto keywordForm = a_object->As<RE::BGSKeywordForm>();
                    bool left = false;
                    if (a_slot->GetFormID() == Utils::EquipSlots::Left) {
                        left = true;
                    }
                    if ((a_object->IsArmor()) || (a_object->Is(RE::FormType::Light))) {
                        left = true;
                    }
                    Utils::SetInventoryInfo(keywordForm, left, true);
                }
                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }

            if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,    a_slot, a_queueEquip,
                                                       a_forceEquip, a_playSounds, a_applyNow, false};

                    
                    logger::trace("UnEquip PC Hook: Add {} To Queue", a_actor->GetName());
                    UpdateQueue(a_actor->GetFormID(), EquipEvent);
                    //Events::CreateEventSink(a_actor);
                    if (a_actor->IsPlayerRef()) {
                        auto keywordForm = a_object->As<RE::BGSKeywordForm>();
                        bool left = false;
                        if (a_slot->GetFormID() == Utils::EquipSlots::Left) {
                            left = true;
                        }
                        if ((a_object->IsArmor()) || (a_object->Is(RE::FormType::Light))) {
                            left = true;
                        }
                        Utils::SetInventoryInfo(keywordForm, left, true);
                    }
                    a_actor->DrawWeaponMagicHands(false);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    return;
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow, a_slotToReplace);
    }

    void UnEquipObjectNPCHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                     RE::ExtraDataList* a_extraData, std::uint32_t a_count,
                                     const RE::BGSEquipSlot* a_slot, bool a_queueEquip, bool a_forceEquip,
                                     bool a_playSounds, bool a_applyNow, const RE::BGSEquipSlot* a_slotToReplace) {
        if (!a_object || !a_actor || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow, a_slotToReplace);
        }
        if (a_actor->IsPlayerRef()) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow, a_slotToReplace);
        }
        if (a_object->IsWeapon()) {
            if ((a_object == Utils::unarmed_weapon) || (a_object->As<RE::TESObjectWEAP>()->IsBound())) {
                return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                            a_playSounds, a_applyNow, a_slotToReplace);
            }
        }

        if (Utils::IsInHand(a_object)) {
            if (Utils::IsInQueue(a_actor->GetFormID())) {
                logger::trace("UnEquip NPC Hook: {} Is In Queue", a_actor->GetName());
                const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,    a_slot, a_queueEquip,
                                                   a_forceEquip, a_playSounds, a_applyNow, false};
                UpdateQueue(a_actor->GetFormID(), EquipEvent);
                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }

            if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    logger::trace("UnEquip NPC Hook: Add {} To Queue", a_actor->GetName());
                    const Utils::EquipEvent EquipEvent{a_object,     a_extraData,  a_count,    a_slot, a_queueEquip,
                                                       a_forceEquip, a_playSounds, a_applyNow, false};
                    UpdateQueue(a_actor->GetFormID(), EquipEvent);
                    //Events::CreateEventSink(a_actor);
                    a_actor->DrawWeaponMagicHands(false);
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                    return;
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow, a_slotToReplace);
    }

    // TBD:: Move it to ActionEvent? kEndSheathe?
    // This may fix the issue with Timeouts
    template <class T>
    void ActorUpdateHook<T>::thunk(T* a_actor, float a_delta) {
        if (!a_actor || !Settings::Mod_Active) {
            return func(a_actor, a_delta);
        }
        if (!Utils::IsInQueue(a_actor->GetFormID())) {
            return func(a_actor, a_delta);
        }
        if (a_actor->IsDead()) {
            // :'(
            Utils::RemoveFromQueue(a_actor->GetFormID());
            return func(a_actor, a_delta);
        }

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
            RE::WEAPON_STATE weaponState = actorState->GetWeaponState();

            if (weaponState != RE::WEAPON_STATE::kSheathed) {
                if (actorState->actorState2.staggered) {
                    Utils::UpdateTimestamp(a_actor->GetFormID());
                }

                // Timeout - Why?
                // Sometimes (Rare, but noticeable) // Actors get stuck, their Weapon State is not updated
                // so they never get Sheathed state even if they finished Shearling
                const float time_since_last_update =
                    (Utils::gameHour->value - Utils::GetTimestamp(a_actor->GetFormID())) / Utils::timescale->value;
                if (time_since_last_update < 0.0006f) {  // Check for timeout (~2s)
                    return func(a_actor, a_delta);
                } else {
                    logger::warn("Timeout for {}", a_actor->GetName());
                    a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;
                }
            }

            logger::trace("Calling all Queued Actions for {}", a_actor->GetName());
            std::queue<Utils::EquipEvent> equipQueue = Utils::GetQueue(a_actor->GetFormID());
            // Remove Actor from Queue to not mess during Equip calls from Queue
            Utils::RemoveFromQueue(a_actor->GetFormID());

            
            if (a_actor->IsPlayerRef()) {
                auto EqObj = a_actor->GetEquippedObject(true);
                if (EqObj) {
                    auto kwdForm = EqObj->As<RE::BGSKeywordForm>();
                    Utils::RemoveInventoryInfo(kwdForm);
                }
                EqObj = a_actor->GetEquippedObject(false);
                if (EqObj) {
                    auto kwdForm = EqObj->As<RE::BGSKeywordForm>();
                    Utils::RemoveInventoryInfo(kwdForm);
                }
            }

            // Execute all Queued actions
            while (!equipQueue.empty()) {
                const Utils::EquipEvent& currEvent = equipQueue.front();
                RE::BGSKeywordForm* kwdForm = nullptr;
                if (currEvent.equip) {
                    if (currEvent.spell) {
                        kwdForm = currEvent.spell->As<RE::BGSKeywordForm>();
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(a_actor, currEvent.spell, currEvent.slot);
                    } else {
                        kwdForm = currEvent.object->As<RE::BGSKeywordForm>();
                        RE::ActorEquipManager::GetSingleton()->EquipObject(
                            a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                            currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                    }
                } else {
                    kwdForm = currEvent.object->As<RE::BGSKeywordForm>();
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(
                        a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                        currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                }
                if (a_actor->IsPlayerRef()) {
                    Utils::RemoveInventoryInfo(kwdForm);
                }
                // Keep kSheathed state until all queue is executed
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;
                equipQueue.pop();
            }
            // No idea why this order matter, but it does
            a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToDraw;
            a_actor->DrawWeaponMagicHands(true);
        }
        return func(a_actor, a_delta);
    }
};
