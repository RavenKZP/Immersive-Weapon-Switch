#include "Events.h"

RE::BSEventNotifyControl EquipEventSink::ProcessEvent(const RE::TESEquipEvent* event,
                                                    RE::BSTEventSource<RE::TESEquipEvent>*) {
    if (event->actor) {
        if (!event->equipped) //Unequip event
        {
            RE::TESForm* un_equiped_object = RE::TESForm::LookupByID(event->baseObject);
            if (un_equiped_object) { //nullptr check 
                logger::debug("{} is unequiping {}", event->actor->GetName(), un_equiped_object->GetName());
                if (un_equiped_object->IsWeapon()) {  // It's a weapon

                    logger::debug("It's a Weapon!");
                    RE::Actor* actor = event->actor->As<RE::Actor>();
                    if (actor) {  // nullptr check 
                        RE::ActorState* actorState = actor->AsActorState();
                        if (actorState) {  // nullptr check 
                            if (actorState->IsWeaponDrawn()) { // He has weapon ready (kDrawn || kWantToSheathe || kSheathing) TBD: Limit it to just kDrawn??
                                logger::debug("And weapon is ready");
                                actor->DrawWeaponMagicHands(false);
                            }
                        }
                    }
                }
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}