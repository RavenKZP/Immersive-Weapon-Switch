#include "MCP.h"
#include "Settings.h"
#include "Utils.h"

namespace Helper {

    std::string WeaponStateToString(const RE::WEAPON_STATE state) {
        switch (state) {
            case RE::WEAPON_STATE::kSheathed:
                return "kSheathed";
            case RE::WEAPON_STATE::kDrawing:
                return "kDrawing";
            case RE::WEAPON_STATE::kDrawn:
                return "kDrawn";
            case RE::WEAPON_STATE::kSheathing:
                return "kSheathing";
            case RE::WEAPON_STATE::kWantToDraw:
                return "kWantToDraw";
            case RE::WEAPON_STATE::kWantToSheathe:
                return "kWantToSheathe";
            default:
                return "Unk";
        }
    }

    std::string GetSlotAsString(const RE::FormID slotID) {
        switch (slotID) {
            case 81730:
                return "Right";
            case 81731:
                return "Left";
            case 81733:
                return "Both";
            case 82408:
                return "Shield";
            default:
                return "Unk";
        }
    }

}  // Helper

namespace Utils {

    static RE::BGSKeyword* FindOrCreateKeyword(std::string kwd) {
        auto form = RE::TESForm::LookupByEditorID(kwd);
        if (form) {
            return form->As<RE::BGSKeyword>();
        }
        logger::warn("No keyword {} found, creating it", kwd);
        const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSKeyword>();
        RE::BGSKeyword* createdkeyword = nullptr;
        if (createdkeyword = factory ? factory->Create() : nullptr; createdkeyword) {
            createdkeyword->formEditorID = kwd;
        }
        return createdkeyword;
    }

    void InitGlobals() {
#undef GetObject
        unarmed_weapon = RE::TESForm::LookupByID(0x1f4)->As<RE::TESBoundObject>();
        right_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kRightHandEquip);
        left_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kLeftHandEquip);

        switch_keyword_left = FindOrCreateKeyword("IWS_SwitchInProgressLeft");
        switch_keyword_right = FindOrCreateKeyword("IWS_SwitchInProgressRight");

        unequip_keyword_left = FindOrCreateKeyword("IWS_UnequipInProgressLeft");
        unequip_keyword_right = FindOrCreateKeyword("IWS_UnequipInProgressRight");

        equip_keyword_left = FindOrCreateKeyword("IWS_EquipInProgressLeft");
        equip_keyword_right = FindOrCreateKeyword("IWS_EquipInProgressRight");

        if (unarmed_weapon && right_hand_slot && left_hand_slot && switch_keyword_right && switch_keyword_left &&
            unequip_keyword_left && unequip_keyword_right && equip_keyword_left && equip_keyword_right) {

            logger::info("Global variables initialized");
        } else {
            logger::error("Error during Initialization of Global variables");
        }
    }

    void UpdateEventInfo(RE::Actor* act, RE::TESBoundObject* object, bool left, bool unequip) {
        logger::debug("UpdateEventInfo: {} {} {} {}", act->GetName(), object->GetName(), left ? "left" : "right",
                      unequip ? "unequip" : "equip");
        if (!object || !act) {
            logger::error("{} is nullptr", act ? "Object" : "Actor");
            return;
        }
        std::unique_lock ulock(actor_equip_event_mutex);

        auto actor_curr_right = act->GetEquippedObject(false);
        auto actor_curr_left = act->GetEquippedObject(true);

        if (const auto it = actor_equip_event.find(act);
            it != actor_equip_event.end()) {  // actor already tracked, udpate event
            logger::debug("{} already tracked", act->GetName());

            RE::TESObjectREFR::Count count = 0;

            if (!object->IsWeapon()) {
                count = 69; //whatever more than 2 :D
            } else {
                RE::TESObjectREFR::InventoryItemMap inv = act->GetInventory();
                auto it_inv = inv.find(object);
                if (it_inv != inv.end()) {
                    count = it_inv->second.first;
                    logger::debug("{} has {} of the {}.", act->GetName(), count, object->GetName());
                } else {
                    logger::error("{} not found in {}'s inventory.", object->GetName(), act->GetName());
                    return;
                }
            }

            if (left) {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_left, true, true);
                }
                if (!unequip) {  // always update if equip call
                    if (act->IsPlayerRef()) {
                        RemoveInventoryInfo(it->second.left);
                        SetInventoryInfo(object, true);
                    }
                    it->second.left = object;
                    it->second.unequip_left = unequip;
                    if (it->second.right == object && count < 2) {
                        it->second.right = nullptr;
                    }
                } else {
                    if (!it->second.left) {  // only update unequip if empty
                        it->second.left = object;
                        it->second.unequip_left = unequip;
                    }
                }
            } else {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_right, false, true);
                }
                if (!unequip) {  // always update if equip call
                    if (act->IsPlayerRef()) {
                        RemoveInventoryInfo(it->second.right);
                        SetInventoryInfo(object, false);
                    }
                    it->second.right = object;
                    it->second.unequip_right = unequip;
                    if (IsTwoHanded(object)) {
                        if (act->IsPlayerRef()) {
                            SetInventoryInfo(actor_curr_left, true, true);
                        }
                        it->second.left = nullptr;
                    }
                    if (it->second.left == object && count < 2) {
                        it->second.left = nullptr;
                    }
                } else {
                    if (!it->second.right) {  // only update unequip if empty
                        it->second.right = object;
                        it->second.unequip_right = unequip;
                    }
                }
            }
            logger::debug("Right {}: {}", it->second.unequip_right ? "unequip" : "equip",
                          it->second.right ? it->second.right->GetName() : "nullptr");
            logger::debug("Left {}: {}", it->second.unequip_left ? "unequip" : "equip",
                          it->second.left ? it->second.left->GetName() : "nullptr");
        } else {  // actor not tracked
            logger::debug("{} not tracked", act->GetName());
            EquipEvent temp_event;
            if (left) {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_left, true, true);
                    SetInventoryInfo(object, true, unequip);
                }
                temp_event.left = object;
                temp_event.unequip_left = unequip;
            } else {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_right, false, true);
                    SetInventoryInfo(object, false, unequip);
                    if (IsTwoHanded(object)) {
                        SetInventoryInfo(actor_curr_left, true, true);
                    }
                }
                temp_event.right = object;
                temp_event.unequip_right = unequip;
            }
            logger::debug("Right {}: {}", temp_event.unequip_right ? "unequip" : "equip",
                          temp_event.right ? temp_event.right->GetName() : "nullptr");
            logger::debug("Left {}: {}", temp_event.unequip_left ? "unequip" : "equip",
                          temp_event.left ? temp_event.left->GetName() : "nullptr");
            actor_equip_event.insert({act, temp_event});
        }
    }

    bool IsAlreadyTracked(RE::Actor* act) {
        std::shared_lock ulock(actor_equip_event_mutex);
        return actor_equip_event.contains(act);
    }

    void RemoveEvent(RE::Actor* act) {
        std::unique_lock ulock(actor_equip_event_mutex);
        if (const auto it = actor_equip_event.find(act); it != actor_equip_event.end()) {
            actor_equip_event.erase(it);
        }
    }

    void RemoveEvent(const RE::Actor* act) {
        std::unique_lock ulock(actor_equip_event_mutex);
        RE::Actor* a_actor = const_cast<RE::Actor*>(act);
        if (const auto it = actor_equip_event.find(a_actor); it != actor_equip_event.end()) {
            actor_equip_event.erase(it);
        }
    }

    void ClearAllEvents() {
        std::unique_lock ulock(actor_equip_event_mutex);
        actor_equip_event.clear();
    }

    EquipEvent GetEvent(RE::Actor* act) {
        std::shared_lock ulock(actor_equip_event_mutex);
        EquipEvent res;
        if (const auto it = actor_equip_event.find(act); it != actor_equip_event.end()) {
            res = it->second;
        }
        return res;
    }

    EquipEvent GetEvent(const RE::Actor* act) {
        std::shared_lock ulock(actor_equip_event_mutex);
        EquipEvent res;
        RE::Actor* a_actor = const_cast<RE::Actor*>(act);
        if (const auto it = actor_equip_event.find(a_actor); it != actor_equip_event.end()) {
            res = it->second;
        }
        return res;
    }

    RE::Actor* GetActor(const RE::Actor* act) {
        RE::Actor* a_actor = RE::TESForm::LookupByID(act->GetFormID())->As<RE::Actor>();
        return a_actor;
    }

    void ExecuteEvent(const RE::Actor* const_act) {
        RE::Actor* actor = GetActor(const_act);
        EquipEvent eqEve = GetEvent(const_act);

        Utils::RemoveEvent(actor);
        
        if (actor->IsPlayerRef()) {
            auto actor_curr_right = actor->GetEquippedObject(false);
            auto actor_curr_left = actor->GetEquippedObject(true);

                RemoveInventoryInfo(actor_curr_right);
            
                RemoveInventoryInfo(actor_curr_left);
            
        }

        actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;

        auto EqManager = RE::ActorEquipManager::GetSingleton();
        if (eqEve.right) {
            if (actor->IsPlayerRef()) {
                RemoveInventoryInfo(eqEve.right);
            }
            logger::debug("{}Equiping right: {} {}", eqEve.unequip_right ? "" : "Un", actor->GetName(),
                          eqEve.right->GetName());
            if (eqEve.right->As<RE::SpellItem>()) {
                EqManager->EquipSpell(actor, eqEve.right->As<RE::SpellItem>(), Utils::right_hand_slot);
            } else {
                if (eqEve.unequip_right) {
                    EqManager->UnequipObject(actor, eqEve.right, nullptr, 1, nullptr, false, false, false, true);
                } else {
                    EqManager->EquipObject(actor, eqEve.right, nullptr, 1, nullptr, false, false, false, true);
                }
            }
        }
        if (eqEve.left) {
            if (actor->IsPlayerRef()) {
                RemoveInventoryInfo(eqEve.left);
            }
            logger::debug("{}Equiping left: {}", eqEve.unequip_left ? "" : "Un", actor->GetName(),
                          eqEve.left->GetName());
            if (eqEve.left->As<RE::SpellItem>()) {
                EqManager->EquipSpell(actor, eqEve.left->As<RE::SpellItem>(), Utils::left_hand_slot);
            } else {
                if (IsLeftOnly(eqEve.left)) {
                    if (eqEve.unequip_left) {
                        EqManager->UnequipObject(actor, eqEve.left, nullptr, 1, nullptr, false, false, false, true);
                    } else {
                        EqManager->EquipObject(actor, eqEve.left, nullptr, 1, nullptr, false, false, false, true);
                    }
                } else {
                    if (eqEve.unequip_left) {
                        EqManager->UnequipObject(actor, eqEve.left, nullptr, 1, Utils::left_hand_slot, false, false,
                                                 false, true);
                    } else {
                        EqManager->EquipObject(actor, eqEve.left, nullptr, 1, Utils::left_hand_slot, false, false,
                                               false, true);
                    }
                }
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(actor, nullptr);

        // No idea why this order matter, but it does
        actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToDraw;
        actor->DrawWeaponMagicHands(true);
    }

    bool IsInHand(RE::TESBoundObject* a_object) {
        if (a_object == nullptr) {  // Empty hand
            return true;
        }
        if (a_object == unarmed_weapon) {  // brawl
            return true;
        }
        if (a_object->Is(RE::FormType::Weapon) || a_object->Is(RE::FormType::Light) ||
            a_object->Is(RE::FormType::Scroll)) {
            return true;
        }
        if (a_object->Is(RE::FormType::Spell)) {
            if (a_object->As<RE::SpellItem>()->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                return true;
            }
        }
        if (a_object->Is(RE::FormType::Armor)) {
            if (const auto equip_slot = a_object->As<RE::TESObjectARMO>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == EquipSlots::Shield) {
                return true;
            }
        }
        return false;
    }

    bool IsWhitelistUnequip(RE::TESBoundObject* a_object) {
        bool res = false;
        if (a_object->Is(RE::FormType::Light) || a_object->Is(RE::FormType::Scroll) ||
            a_object->Is(RE::FormType::Spell)) {
            res = true;
        }
        if (a_object->IsWeapon() && a_object->As<RE::TESObjectWEAP>()->IsBound()) {
            res = true;
        }
        if (a_object->Is(RE::FormType::Armor)) {
            res = true;
        }
        return res;
    }

    bool IsTwoHanded(RE::TESForm* a_object) {
        if (a_object) {
            if ((a_object->IsWeapon()) &&
                ((a_object->As<RE::TESObjectWEAP>()->IsBow()) || (a_object->As<RE::TESObjectWEAP>()->IsCrossbow()) ||
                 (a_object->As<RE::TESObjectWEAP>()->IsTwoHandedAxe()) ||
                 (a_object->As<RE::TESObjectWEAP>()->IsTwoHandedSword()))) {
                return true;
            }
        }
        return false;
    }

    bool IsTwoHanded(RE::TESBoundObject* a_object) {
        if (a_object) {
            if ((a_object->IsWeapon()) &&
                ((a_object->As<RE::TESObjectWEAP>()->IsBow()) || 
                 (a_object->As<RE::TESObjectWEAP>()->IsCrossbow()) ||
                 (a_object->As<RE::TESObjectWEAP>()->IsTwoHandedAxe()) ||
                 (a_object->As<RE::TESObjectWEAP>()->IsTwoHandedSword()))) {
                return true;
            }
        }
        return false;
    }

    bool IsLeftOnly(RE::TESBoundObject* a_object) {
        bool res = false;
        if (a_object->Is(RE::FormType::Armor)) {
            if (const auto equip_slot = a_object->As<RE::TESObjectARMO>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Shield) {
                res = true;
            }
        }
        if (a_object->Is(RE::FormType::Light)) {
            res = true;
        }
        if (a_object->IsWeapon()) {
            if (a_object->As<RE::TESObjectWEAP>()->GetEquipSlot() == Utils::left_hand_slot) {
                res = true;
            }
        }
        return res;
    }

    template <typename T>
    void SetInventoryInfo(T* obj, bool left, bool unequip) {
        if (!obj) {
            return;
        }
        RE::BGSKeywordForm* kwdForm = obj->As<RE::BGSKeywordForm>();
        if (unequip) {
            if (left) {
                if (kwdForm && unequip_keyword_left) {
                    if (!kwdForm->HasKeyword(unequip_keyword_left)) {
                        kwdForm->AddKeyword(unequip_keyword_left);
                    }
                }
            } else {
                if (kwdForm && unequip_keyword_right) {
                    if (!kwdForm->HasKeyword(unequip_keyword_right)) {
                        kwdForm->AddKeyword(unequip_keyword_right);
                    }
                }
            }
        } else {
            if (left) {
                if (kwdForm && switch_keyword_left) {
                    if (!kwdForm->HasKeyword(switch_keyword_left)) {
                        kwdForm->AddKeyword(switch_keyword_left);
                    }
                }
            } else {
                if (kwdForm && switch_keyword_right) {
                    if (!kwdForm->HasKeyword(switch_keyword_right)) {
                        kwdForm->AddKeyword(switch_keyword_right);
                    }
                }
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(RE::PlayerCharacter::GetSingleton(), nullptr);
    }

    template <typename T>
    void RemoveInventoryInfo(T* obj) {
        if (!obj) {
            return;
        }
        RE::BGSKeywordForm* kwdForm = obj->As<RE::BGSKeywordForm>();
        if (kwdForm && switch_keyword_left) {
            if (kwdForm->HasKeyword(switch_keyword_left)) {
                kwdForm->RemoveKeyword(switch_keyword_left);
            }
        }
        if (kwdForm && switch_keyword_right) {
            if (kwdForm->HasKeyword(switch_keyword_right)) {
                kwdForm->RemoveKeyword(switch_keyword_right);
            }
        }
        if (kwdForm && unequip_keyword_left) {
            if (kwdForm->HasKeyword(unequip_keyword_left)) {
                kwdForm->RemoveKeyword(unequip_keyword_left);
            }
        }
        if (kwdForm && unequip_keyword_right) {
            if (kwdForm->HasKeyword(unequip_keyword_right)) {
                kwdForm->RemoveKeyword(unequip_keyword_right);
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(RE::PlayerCharacter::GetSingleton(), nullptr);
    }

}  // Utils
