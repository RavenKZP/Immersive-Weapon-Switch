#include "Events.h"
#include "Utils.h"

    RE::BSEventNotifyControl
    EquipEventSink::ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*) {
    if (event->actor) {
        if (!event->equipped)  // Unequip event
        {
            RE::TESForm* un_equiped_object = RE::TESForm::LookupByID(event->baseObject);
            if (un_equiped_object) {  // nullptr check
                logger::debug("{} is unequiping {}", event->actor->GetName(), un_equiped_object->GetName());
                if (un_equiped_object->IsWeapon()) {  // It's a weapon

                    RE::Actor* actor = event->actor->As<RE::Actor>();
                    if (actor) {  // nullptr check
                        RE::ActorState* actorState = actor->AsActorState();
                        if (actorState) {  // nullptr check
                            if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                                logger::debug("{} has Drawn Weapon, undrawing first {}", event->actor->GetName(),
                                              un_equiped_object->GetName());
                                actor->DrawWeaponMagicHands(false);
                                Utils::UpdatePospondEquipQueue(actor, nullptr);
                                return RE::BSEventNotifyControl::kStop;
                            }
                        }
                    }
                }
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}