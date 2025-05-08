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
        logger::trace("UpdateEventInfo: {} {}", act->GetName(), object->GetName());
        std::unique_lock ulock(actor_equip_event_mutex);
        if (const auto it = actor_equip_event.find(act);
            it != actor_equip_event.end()) {  // actor already tracked, udpate event
            logger::trace("actor already tracked, udpate event");
            if (left) {
                if (!unequip) {  // always update if equip call
                    it->second.left = object;
                    it->second.unequip_left = unequip;
                } else {
                    if (!it->second.left) {  // only update unequip if empty
                        it->second.left = object;
                        it->second.unequip_left = unequip;
                    }
                }
            } else {
                if (!unequip) {  // always update if equip call
                    it->second.right = object;
                    it->second.unequip_right = unequip;
                    if (IsTwoHanded(object)) {
                        it->second.left = nullptr;
                        it->second.unequip_left = true;
                    }
                } else {
                    if (!it->second.left) {  // only update unequip if empty
                        it->second.right = object;
                        it->second.unequip_right = unequip;
                    }
                }
            }
            logger::trace("Event state:");
            logger::trace("Right: {}", it->second.right ? it->second.right->GetName() : "nullptr");
            logger::trace("Left: {}", it->second.left ? it->second.left->GetName() : "nullptr");
        } else {  // actor not tracked
            logger::trace("actor not tracked");
            EquipEvent temp_event;
            if (left) {
                temp_event.left = object;
                temp_event.unequip_left = unequip;
            } else {
                temp_event.right = object;
                temp_event.unequip_right = unequip;
            }
            logger::trace("Event state:");
            logger::trace("Right: {}", temp_event.right ? temp_event.right->GetName() : "nullptr");
            logger::trace("Left: {}", temp_event.left ? temp_event.left->GetName() : "nullptr");
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

        actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;

        auto EqManager = RE::ActorEquipManager::GetSingleton();
        if (eqEve.right) {
            logger::trace("Equiping right: {} {}", actor->GetName(), eqEve.right->GetName());
            if (eqEve.right->As<RE::SpellItem>()) {
                EqManager->EquipSpell(actor, eqEve.right->As<RE::SpellItem>(), Utils::right_hand_slot);
            } else {
                if (eqEve.unequip_right) {
                    EqManager->UnequipObject(actor, eqEve.right, nullptr, 1, Utils::right_hand_slot, false);
                } else {
                    EqManager->EquipObject(actor, eqEve.right, nullptr, 1, Utils::right_hand_slot, false);
                }
            }
        }
        if (eqEve.left) {
            logger::trace("Equiping left: {}", actor->GetName(), eqEve.left->GetName());
            if (eqEve.left->As<RE::SpellItem>()) {
                EqManager->EquipSpell(actor, eqEve.left->As<RE::SpellItem>(), Utils::left_hand_slot);
            } else {
                if (IsLeftOnly(eqEve.left)) {
                    if (eqEve.unequip_left) {
                        EqManager->UnequipObject(actor, eqEve.left, nullptr, 1, Utils::right_hand_slot, false);
                    } else {
                        EqManager->EquipObject(actor, eqEve.left, nullptr, 1, Utils::right_hand_slot, false);
                    }
                } else {
                    if (eqEve.unequip_left) {
                        EqManager->UnequipObject(actor, eqEve.left, nullptr, 1, Utils::left_hand_slot, false);
                    } else {
                        EqManager->EquipObject(actor, eqEve.left, nullptr, 1, Utils::left_hand_slot, false);
                    }
                }
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(actor, nullptr);

        // No idea why this order matter, but it does
        actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToDraw;
        actor->DrawWeaponMagicHands(true);

        Utils::RemoveEvent(actor);
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
        return res;
    }

    bool IsTwoHanded(RE::TESBoundObject* a_object) {
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
        return res;
    }

    void SetInventoryInfo(RE::BGSKeywordForm* kwdForm, bool left, bool unequip) {
        static RE::BGSKeywordForm* previous_left = nullptr;
        static RE::BGSKeywordForm* previous_right = nullptr;
        auto player = RE::PlayerCharacter::GetSingleton();

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
            return;
        }

        if (left && previous_left) {
            RemoveInventoryInfo(previous_left);
        }
        if (!left && previous_right) {
            RemoveInventoryInfo(previous_right);
        }

        if (left) {
            previous_left = kwdForm;
            if (kwdForm && switch_keyword_left) {
                if (!kwdForm->HasKeyword(switch_keyword_left)) {
                    kwdForm->AddKeyword(switch_keyword_left);
                }
            }
        } else {
            previous_right = kwdForm;
            if (kwdForm && switch_keyword_right) {
                if (!kwdForm->HasKeyword(switch_keyword_right)) {
                    kwdForm->AddKeyword(switch_keyword_right);
                }
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(player, nullptr);
    }

    void RemoveInventoryInfo(RE::BGSKeywordForm* kwdForm) {
        auto player = RE::PlayerCharacter::GetSingleton();
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
        RE::SendUIMessage::SendInventoryUpdateMessage(player, nullptr);
    }

}  // Utils
