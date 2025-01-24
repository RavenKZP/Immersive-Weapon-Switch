#pragma once
#include <shared_mutex>

namespace Helper {

    std::string WeaponStateToString(RE::WEAPON_STATE state);
    std::string GetSlotAsString(RE::FormID slotID);
}

namespace Utils {

    struct EquipEvent {
        RE::TESBoundObject* object = nullptr;
        RE::ExtraDataList* extraData = nullptr;
        std::uint32_t count = 1;
        const RE::BGSEquipSlot* slot = nullptr;
        bool queueEquip = true;
        bool forceEquip = false;
        bool playSounds = true;
        bool applyNow = false;
        bool equip = true;

        //Spells use Actor, SpellItem, BGSEquipSlot
        RE::SpellItem* spell = nullptr;
    };

    const enum EquipSlots { RightHand = 81730, LeftHand = 81731, Shield = 82408
    };
    
    void UpdateQueue(RE::FormID actID, const EquipEvent& equipdata);
    bool IsInQueue(RE::FormID actID);
    void RemoveFromQueue(RE::FormID actID);
    void ClearQueue();
    std::queue<EquipEvent> GetQueue(RE::FormID actID);

    //Returns true whenever given object is equipable in left, right or both hands
    bool IsInHand(RE::TESBoundObject* a_object);
    //Returns True whenever hand is empty or equiped with scroll/spell
    bool IsHandFree(RE::FormID slotID, RE::Actor* actor);
    
	inline std::shared_mutex actor_queue_mutex;
    inline std::unordered_map<RE::FormID, std::queue<EquipEvent>> actor_queue;
}
