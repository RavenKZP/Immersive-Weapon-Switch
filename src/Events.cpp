#include "Events.h"
#include "Settings.h"
#include "Utils.h"

namespace Events {

    AnimationEventSink::AnimationEventSink(RE::Actor* actor) : a_actor(actor) {}

    RE::BSEventNotifyControl AnimationEventSink::ProcessEvent(const SKSE::ActionEvent* a_event,
                                                              RE::BSTEventSource<SKSE::ActionEvent>*) {
        
        if (a_event && a_actor) {
            if (!Settings::Mod_Active) {
                logger::trace("Mod Inactive - Removing event sink for Actor: '{}'", a_actor->GetName());
                Utils::RemoveFromQueue(a_actor->GetFormID());
                RemoveEventSink(this);
                return RE::BSEventNotifyControl::kContinue;
            }
            if (a_actor->IsDead()) {
                // :'(
                logger::trace("Actor Is Dead - Removing event sink for Actor: '{}'", a_actor->GetName());
                Utils::RemoveFromQueue(a_actor->GetFormID());
                RemoveEventSink(this);
                return RE::BSEventNotifyControl::kContinue;
            }

            if (a_event->type == SKSE::ActionEvent::Type::kEndSheathe) {
                std::queue<Utils::EquipEvent> equipQueue = Utils::GetQueue(a_actor->GetFormID());
                // Remove Actor from Queue to not mess during Equip calls from Queue
                Utils::RemoveFromQueue(a_actor->GetFormID());
                // Execute all Queued actions
                while (!equipQueue.empty()) {
                    if (const Utils::EquipEvent& currEvent = equipQueue.front(); currEvent.equip) {
                        if (currEvent.spell) {
                            RE::ActorEquipManager::GetSingleton()->EquipSpell(a_actor, currEvent.spell, currEvent.slot);
                        } else {
                            if (a_actor->IsPlayerRef()) {
                                //Utils::RemoveInventoryInfo();
                            }
                            RE::ActorEquipManager::GetSingleton()->EquipObject(
                                a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                                currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                        }
                    } else {
                        if (a_actor->IsPlayerRef()) {
                            //Utils::RemoveInventoryInfo();
                        }
                        RE::ActorEquipManager::GetSingleton()->UnequipObject(
                            a_actor, currEvent.object, currEvent.extraData, currEvent.count, currEvent.slot,
                            currEvent.queueEquip, currEvent.forceEquip, currEvent.playSounds, currEvent.applyNow);
                    }
                    equipQueue.pop();
                }
                // No idea why this order matter, but it does
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToDraw;
                a_actor->DrawWeaponMagicHands(true);
                logger::trace("Removing event sink for Actor: '{}'", a_actor->GetName());
                RemoveEventSink(this);
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    void CreateEventSink(RE::Actor* a_actor) {
        AnimationEventSink* eventSink = nullptr;
        if (!a_actor) {
            return;
        }
        if (const auto it = actorEventSinks.find(a_actor->GetFormID()); it != actorEventSinks.end()) {
            eventSink = it->second.get();
        } else {
            eventSink = new AnimationEventSink(a_actor);
            actorEventSinks.emplace(a_actor->GetFormID(), eventSink);
        }
        RE::BSTEventSource<SKSE::ActionEvent>* ActionEventSource = SKSE::GetActionEventSource();
        ActionEventSource->AddEventSink(eventSink);
    }

    void RemoveEventSink(AnimationEventSink* sink) {
        if (!sink) {
            return;
        }
        RE::BSTEventSource<SKSE::ActionEvent>* ActionEventSource = SKSE::GetActionEventSource();
        ActionEventSource->RemoveEventSink(sink);

        if (const auto it = actorEventSinks.find(sink->a_actor->GetFormID()); it != actorEventSinks.end()) {
            actorEventSinks.erase(it);
        }
        logger::trace("AnimationEventSink deleted.");
    }
}
