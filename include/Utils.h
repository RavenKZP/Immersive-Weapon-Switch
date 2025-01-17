#ifndef UTILS_H
#define UTILS_H

#include <functional>
#include <shared_mutex>
#include<unordered_set>

namespace Helper {

    std::string WeaponStateToString(RE::WEAPON_STATE state);
}


namespace Utils {

    struct ActorInfo {
        FormID baseID;
        RefID refID;
        RE::TESBoundObject* queued_weap = nullptr;
        bool unequip = false;
        bool left = false;

        bool operator<(const ActorInfo& rhs) const {
            return baseID < rhs.baseID || (baseID == rhs.baseID && refID < rhs.refID);
        }

        bool operator==(const ActorInfo& rhs) const { return baseID == rhs.baseID && refID == rhs.refID; }

        explicit ActorInfo(RE::Actor* a_this) {
            baseID = a_this->GetBaseObject()->GetFormID();
            refID = a_this->GetFormID();
        }

        explicit ActorInfo(RE::Actor* a_this, RE::TESBoundObject* a_weap, bool unequip_flag = false, bool left_hand = false) {
            baseID = a_this->GetBaseObject()->GetFormID();
            refID = a_this->GetFormID();
            queued_weap = a_weap;
            unequip = unequip_flag;
            left = left_hand;
        }
    };
}

namespace std {
    template <>
    struct hash<Utils::ActorInfo> {
        size_t operator()(const Utils::ActorInfo& info) const noexcept {
            size_t ref_hash = std::hash<RefID>{}(info.refID);
            size_t base_hash = std::hash<FormID>{}(info.baseID);
            return ref_hash ^ (base_hash << 1);
        }
    };
}

namespace Utils {
    
    void UpdatePospondEquipQueue(RE::Actor* a_actor, RE::TESBoundObject* a_object, bool unqeuip_flag = false, bool left_hand = false);
    bool IsInPospondEquipQueue(ActorInfo actInf);
    void RemoveFromPospondEquipQueue(ActorInfo actInf);
    void ClearPospondEquipQueue();
    RE::TESBoundObject* GetObjectFromPospondEquipQueue(ActorInfo actInf);
    bool GetUnequipFromPospondEquipQueue(ActorInfo actInf);
    
	inline std::shared_mutex actor_queue_mutex;
    inline std::unordered_set<ActorInfo> actor_queue;
}

#endif  // UTILS_H