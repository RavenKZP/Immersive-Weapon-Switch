#include "Utils.h"

namespace Helper {

    std::string WeaponStateToString(RE::WEAPON_STATE state) {
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

    std::string GetSlotAsString(RE::FormID slotID) { 
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
        }
        return res;
    }
}

namespace Utils {

    void UpdateQueue(RE::FormID actID, EquipEvent equipdata) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) { // actor already tracked, add next event to the queue
            it->second.push(equipdata);
        } else { // actor not tracked
            std::queue<EquipEvent> temp_queue;
            temp_queue.push(equipdata);
            actor_queue.insert({actID, temp_queue});
        }
    }

    bool IsInQueue(RE::FormID actID) {
        std::unique_lock ulock(actor_queue_mutex);
        bool res = false;
        if (actor_queue.contains(actID)) {
            res = true;
        }
        return res;
    }

    void RemoveFromQueue(RE::FormID actID) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            actor_queue.erase(it);
        }
    }

    void ClearQueue() {
        std::unique_lock ulock(actor_queue_mutex);
        actor_queue.clear();
    }

    std::queue<EquipEvent> GetQueue(RE::FormID actID) {
        std::unique_lock ulock(actor_queue_mutex);
        std::queue<EquipEvent> res;
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            res = it->second;
        }
        return res;
    }

    bool IsInHand(RE::TESBoundObject* a_object) {
        bool res = false;
        if (a_object->IsWeapon()) {
            res = true;
        } else if (a_object->IsArmor() && a_object->As<RE::TESObjectARMO>()->GetEquipSlot()->formID == 82408) { 
            res = true;
        } else if (a_object->Is(RE::FormType::Light)) { //Equipable Lights 
            res = true;
        } else if (a_object->Is(RE::FormType::Scroll)) {
            res = true;
        }
        return res;
    }
}