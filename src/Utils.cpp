#include "Utils.h"

namespace Helper {

    std::string WeaponStateToString(const RE::WEAPON_STATE state) {
        std::string res = "Unk";
        if (state == RE::WEAPON_STATE::kSheathed) {
            res = "kSheathed";
        }
        if (state == RE::WEAPON_STATE::kDrawing) {
            res = "kDrawing";
        }
        if (state == RE::WEAPON_STATE::kDrawn) {
            res = "kDrawn";
        }
        if (state == RE::WEAPON_STATE::kSheathing) {
            res = "kSheathing";
        }
        if (state == RE::WEAPON_STATE::kWantToDraw) {
            res = "kWantToDraw";
        }
        if (state == RE::WEAPON_STATE::kWantToSheathe) {
            res = "kWantToSheathe";
        }
        return res;
    }

    std::string GetSlotAsString(const RE::FormID slotID) { 
        std::string res = "Unk";
        switch (slotID) {
            case (81730):
                res = "Right";
                break;
            case (81731):
                res = "Left";
                break;
            case (81733):
                res = "Both";
                break;
            case (82408):
                res = "Shield";
                break;
            default: 
                break;
        }
        return res;
    }
}

namespace Utils {

    void UpdateQueue(RE::FormID actID, const EquipEvent& equipdata) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) { // actor already tracked, add next event to the queue
            it->second.push(equipdata);
        } else { // actor not tracked
            std::queue<EquipEvent> temp_queue;
            temp_queue.push(equipdata);
            actor_queue.insert({actID, temp_queue});
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
            res = it->second;
        }
        return res;
    }

    bool IsInHand(RE::TESBoundObject* a_object) {
        if (a_object == nullptr) { // Empty hand
            return true;
        }
        if (const auto a_formtype = a_object->GetFormType();
            a_formtype == RE::FormType::Weapon ||
            a_formtype == RE::FormType::Light ||
            a_formtype == RE::FormType::Scroll
            ) {
			return true;
        }
		else if (a_formtype == RE::FormType::Armor) {
			if (const auto equip_slot = a_object->As<RE::TESObjectARMO>()->GetEquipSlot(); 
                equip_slot && equip_slot->GetFormID() == EquipSlots::Shield) {
				return true;
			}
		}
		return false;
    }

    bool IsHandFree(RE::FormID slotID, RE::Actor* actor) {
        const RE::TESForm* RightHandObj = actor->GetEquippedObject(false);
        const RE::TESForm* LeftHandObj = actor->GetEquippedObject(true);

        if ((slotID == EquipSlots::LeftHand) && (LeftHandObj == nullptr)) {  // Left hand Equip when Left hand empty
            return true;
        }
        if ((slotID == EquipSlots::Shield) && (LeftHandObj == nullptr)) { // Left shield Equip when Left hand empty
            return true;
        }
        if ((slotID == EquipSlots::RightHand) && (RightHandObj == nullptr)) { // Left hand Equip when Right hand empty
            return true;
        }
        if ((slotID == EquipSlots::LeftHand) && (LeftHandObj)) {  // Left hand not empty
            if (LeftHandObj->Is(RE::FormType::Spell)) {
                return true;
            }
            if (LeftHandObj->Is(RE::FormType::Scroll)) {
                return true;
            }
        }
        if ((slotID == EquipSlots::RightHand) && (RightHandObj)) {  // Right hand not empty
            if (RightHandObj->Is(RE::FormType::Spell)) {
                return true;
            }
            if (RightHandObj->Is(RE::FormType::Scroll)) {
                return true;
            }
        }
        return false;
    }
}