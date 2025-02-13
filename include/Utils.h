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

    void UpdateQueue(RE::FormID actID, const EquipEvent& equipdata, bool updateLastObj = true);
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
    // Returns True whenever given object is two handed weapon
    bool IsTwoHanded(RE::TESBoundObject* a_object);
    // Returns true if dropped
    bool DropIfLowHP(RE::Actor* actor);

    EquipSlots SetEquipSlot(RE::TESBoundObject* a_object, RE::Actor* a_actor);

    void SetInventoryInfo(RE::BGSKeywordForm* kwdForm, bool left, bool unequip = false);
    void RemoveInventoryInfo(RE::BGSKeywordForm* kwdForm);

    inline RE::TESGlobal* gameHour;
    inline RE::TESGlobal* timescale;

    inline RE::BGSEquipSlot* right_hand_slot;
    inline RE::BGSEquipSlot* left_hand_slot;

    inline RE::TESBoundObject* unarmed_weapon;

    inline RE::BGSKeyword* switch_keyword_left;
    inline RE::BGSKeyword* switch_keyword_right;
    inline RE::BGSKeyword* unequip_keyword;

    inline std::shared_mutex actor_queue_mutex;
    inline std::unordered_map<RE::FormID, EquipQueueData> actor_queue;

}

