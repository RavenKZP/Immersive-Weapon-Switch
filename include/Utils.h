#pragma once
#include <shared_mutex>
#include <unordered_set>

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

        // Spells use Actor, SpellItem, BGSEquipSlot
        RE::SpellItem* spell = nullptr;
    };

    struct EquipQueueData {
        std::queue<EquipEvent> queue;
        RE::TESBoundObject* last_object;
        float timestamp;
    };

    const enum EquipSlots { Shield = 82408, Right = 81730, Left = 81731, Both = 81733 };

    void InitGlobals();

    void UpdateQueue(RE::FormID actID, const EquipEvent& equipdata);
    bool IsInQueue(RE::FormID actID);
    void RemoveFromQueue(RE::FormID actID);
    void ClearQueue();
    std::queue<EquipEvent> GetQueue(RE::FormID actID);
    float GetTimestamp(RE::FormID actID);
    RE::TESBoundObject* GetLastObject(RE::FormID actID);
    void UpdateTimestamp(RE::FormID actID);

    // Returns true whenever given object is equipable in left, right or both hands
    bool IsInHand(RE::TESBoundObject* a_object);
    // Returns True whenever hand is empty or equiped with scroll/spell
    bool IsHandFree(RE::FormID slotID, RE::Actor* actor, RE::TESBoundObject* a_object);
    // Returns true if dropped
    bool DropIfLowHP(RE::Actor* actor);

    EquipSlots SetEquipSlot(RE::TESBoundObject* a_object, RE::Actor* a_actor);

    void UpdateEquipingInfo(RE::Actor* actor, RE::TESBoundObject* object, RE::FormID slotID, RE::SpellItem* spell = nullptr);
    void RemoveEquipingInfo(RE::Actor* actor);

    inline RE::TESGlobal* gameHour;
    inline RE::TESGlobal* timescale;

    inline std::shared_mutex actor_queue_mutex;
    inline std::unordered_map<RE::FormID, EquipQueueData> actor_queue;

    inline bool ShowSwitchIcons = false;
    inline std::string OldObj1;
    inline std::string OldObj2;
    inline std::string NewObj1;
    inline std::string NewObj2;

    inline const RE::BGSEquipSlot* right_hand_slot;
    inline const RE::BGSEquipSlot* left_hand_slot;
    inline RE::TESBoundObject* unarmed_weapon;
}

