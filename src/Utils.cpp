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

        // Needed for timeout calculation
        gameHour = RE::TESForm::LookupByEditorID("gamehour")->As<RE::TESGlobal>();
        timescale = RE::TESForm::LookupByEditorID("timescale")->As<RE::TESGlobal>();

        switch_keyword_left = FindOrCreateKeyword("IWS_SwitchInProgressLeft");
        switch_keyword_right = FindOrCreateKeyword("IWS_SwitchInProgressRight");
        unequip_keyword_left = FindOrCreateKeyword("IWS_UnequipInProgressLeft");
        unequip_keyword_right = FindOrCreateKeyword("IWS_UnequipInProgressRight");
        equip_keyword_left = FindOrCreateKeyword("IWS_EquipInProgressLeft");
        equip_keyword_right = FindOrCreateKeyword("IWS_EquipInProgressRight");
        

        if (unarmed_weapon && right_hand_slot && left_hand_slot && gameHour && timescale && switch_keyword_right &&
            switch_keyword_left) {
            logger::info("Global variables initialized");
        } else {
            logger::error("Error during Initialization of Global variables");
        }
    }

    void UpdateQueue(RE::FormID actID, const EquipEvent& equipdata, bool updateLastObj) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID);
            it != actor_queue.end()) {  // actor already tracked, add next event to the queue
            it->second.queue.push(equipdata);
            if (updateLastObj && (it->second.last_object != equipdata.object)) {
                it->second.last_object = equipdata.object;
                it->second.timestamp = gameHour->value;
            }
        } else {  // actor not tracked
            std::queue<EquipEvent> temp_queue;
            temp_queue.push(equipdata);
            actor_queue.insert({actID, EquipQueueData{temp_queue, equipdata.object, gameHour->value}});
        }
    }

    bool IsInQueue(const RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        return actor_queue.contains(actID);
    }

    void RemoveFromQueue(const RE::FormID actID) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            actor_queue.erase(it);
        }
    }

    void ClearQueue() {
        std::unique_lock ulock(actor_queue_mutex);
        actor_queue.clear();
    }

    std::queue<EquipEvent> GetQueue(const RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        std::queue<EquipEvent> res;
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            res = it->second.queue;
        }
        return res;
    }

    void UpdateTimestamp(RE::FormID actID) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            it->second.timestamp = gameHour->value;
        }
    }

    float GetTimestamp(RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        float res = 0.0f;
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            res = it->second.timestamp;
        }
        return res;
    }

    RE::TESBoundObject* GetLastObject(RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        RE::TESBoundObject* res = nullptr;
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            res = it->second.last_object;
        }
        return res;
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

    bool IsHandFree(RE::FormID slotID, RE::Actor* actor, RE::TESBoundObject* a_object) {
        bool right_empty = false;
        bool left_empty = false;
        RE::TESForm* RightHandObj = actor->GetEquippedObject(false);
        const RE::TESForm* LeftHandObj = actor->GetEquippedObject(true);

        if (a_object && ((a_object == RightHandObj) || (a_object == LeftHandObj))) {
            return true;
        }

        if (LeftHandObj == nullptr) {
            left_empty = true;
        } else if (LeftHandObj == unarmed_weapon) {
            left_empty = true;
        } else if (LeftHandObj->Is(RE::FormType::Spell)) {
            left_empty = true;
        } else if (LeftHandObj->Is(RE::FormType::Scroll)) {
            left_empty = true;
        } else if (LeftHandObj->Is(RE::FormType::Light)) {
            left_empty = true;
        } else if (LeftHandObj->IsWeapon() && LeftHandObj->As<RE::TESObjectWEAP>()->IsBound()) {
            left_empty = true;
        }

        if (RightHandObj == nullptr) {
            right_empty = true;
        } else if (RightHandObj == unarmed_weapon) {
            right_empty = true;
        } else if (RightHandObj->Is(RE::FormType::Spell)) {
            right_empty = true;
        } else if (RightHandObj->Is(RE::FormType::Scroll)) {
            right_empty = true;
        } else if (RightHandObj->IsWeapon() && RightHandObj->As<RE::TESObjectWEAP>()->IsBound()) {
            right_empty = true;
        }

        // If Right Hand is not empty and it's two handed than Left is not empty too
        if (RightHandObj) {
            if (!right_empty && IsTwoHanded(RightHandObj->As<RE::TESBoundObject>())) {
                left_empty = false;
            }
        }

        // For example Bows and Crossbows Requested slot is Right, when it should be both hands
        if (a_object) {
            if (IsTwoHanded(a_object)) {
                slotID = EquipSlots::Both;
            }
        }
        // For Shields and Lights Requested slot is Right, when it should be Left
        if (a_object) {
            if (a_object->IsArmor() || a_object->Is(RE::FormType::Light)) {
                slotID = EquipSlots::Left;
            }
        }

        logger::trace("IsHandFree Right {} Left {}, Requesetd {}", right_empty, left_empty,
                      Helper::GetSlotAsString(slotID));

        switch (slotID) {
            case EquipSlots::Left:
            case EquipSlots::Shield: {
                return left_empty;
            }
            case EquipSlots::Right: {
                return right_empty;
            }
            case EquipSlots::Both: {
                return (right_empty && left_empty);
            }
            default: {
                return false;
            }
        }
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

    EquipSlots SetEquipSlot(RE::TESBoundObject* a_object, RE::Actor* a_actor) {
        EquipSlots slot = EquipSlots::Right;
        if (a_object->IsArmor()) {
            slot = EquipSlots::Shield;
        } else if (a_object->Is(RE::FormType::Light)) {
            slot = EquipSlots::Left;
        } else if (a_object->IsWeapon()) {
            switch (a_object->As<RE::TESObjectWEAP>()->GetWeaponType()) {
                case (RE::WeaponTypes::WEAPON_TYPE::kTwoHandAxe):
                case (RE::WeaponTypes::WEAPON_TYPE::kTwoHandSword):
                case (RE::WeaponTypes::WEAPON_TYPE::kBow):
                case (RE::WeaponTypes::WEAPON_TYPE::kCrossbow): {
                    slot = EquipSlots::Both;
                    break;
                }
                default: {
                    const RE::TESForm* RightHandObj = a_actor->GetEquippedObject(false);
                    if (IsHandFree(EquipSlots::Right, a_actor, a_object)) {
                        slot = EquipSlots::Right;
                    } else if (IsHandFree(EquipSlots::Left, a_actor, a_object)) {
                        slot = EquipSlots::Left;
                    } else if (RightHandObj == a_object) {
                        slot = EquipSlots::Left;
                    }
                    break;
                }
            }
        }
        return slot;
    }

    bool DropIfLowHP(RE::Actor* a_actor) {
        // Until I fix all bugs
        return false;
        bool res = false;
        float DropHP = 0.0f;
        // Player
        if (a_actor->IsPlayerRef() && Settings::PC_Drop_Weapons) {
            DropHP = Settings::PC_Health_Drop;
        }
        // Follower NPC
        if (!a_actor->IsPlayerRef() && a_actor->IsPlayerTeammate() && Settings::Followers_Drop_Weapons) {
            DropHP = Settings::Followers_Health_Drop;
        }
        // No Follower NPC
        if (!a_actor->IsPlayerRef() && !a_actor->IsPlayerTeammate() && Settings::NPC_Drop_Weapons) {
            DropHP = Settings::NPC_Health_Drop;
        }
        if (DropHP == 0.0f) {  // If no match than terurn
            return res;
        }

        const float curr_hp = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
        const float max_hp = a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHealth);
        if (max_hp != 0.0) {
            if ((curr_hp / max_hp) * 100.0f <= DropHP) {
                RE::TESForm* Left = a_actor->GetEquippedObject(true);
                RE::TESForm* Right = a_actor->GetEquippedObject(false);
                if (Left == Right) {
                    logger::trace("{} dropping {}", a_actor->GetName(), Left->GetName());
                    RE::TESBoundObject* bound = Left->As<RE::TESBoundObject>();
                    a_actor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
                    return true;
                }
                if (Right) {
                    logger::trace("{} dropping {}", a_actor->GetName(), Right->GetName());
                    RE::TESBoundObject* bound = Right->As<RE::TESBoundObject>();
                    a_actor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
                    res = true;
                }
                if (Left) {
                    logger::trace("{} dropping {}", a_actor->GetName(), Left->GetName());
                    RE::TESBoundObject* bound = Left->As<RE::TESBoundObject>();
                    a_actor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
                    res = true;
                }
            }
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
                AddDrawingInfo(kwdForm, true);
            }
        }
        if (kwdForm && unequip_keyword_right) {
            if (kwdForm->HasKeyword(unequip_keyword_right)) {
                kwdForm->RemoveKeyword(unequip_keyword_right);
                AddDrawingInfo(kwdForm, false);
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(player, nullptr);
    }

    void AddDrawingInfo(RE::BGSKeywordForm* kwdForm, bool left) {
        if (left) {
            if (kwdForm && equip_keyword_left) {
                if (!kwdForm->HasKeyword(equip_keyword_left)) {
                    kwdForm->AddKeyword(equip_keyword_left);
                }
            }
        } else {
            if (kwdForm && equip_keyword_right) {
                if (!kwdForm->HasKeyword(equip_keyword_right)) {
                    kwdForm->AddKeyword(equip_keyword_right);
                }
            }
        }
    }

    void RemoveDrawingInfo(RE::BGSKeywordForm* kwdForm) {
        if (kwdForm && equip_keyword_left) {
            if (kwdForm->HasKeyword(equip_keyword_left)) {
                kwdForm->RemoveKeyword(equip_keyword_left);
            }
        }
        if (kwdForm && equip_keyword_right) {
            if (kwdForm->HasKeyword(equip_keyword_right)) {
                kwdForm->RemoveKeyword(equip_keyword_right);
            }
        }
    }

} // Utils
