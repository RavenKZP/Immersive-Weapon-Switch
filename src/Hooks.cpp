#include "Hooks.h"


namespace Hooks {

    void ReadyWeaponHandlerHook::InstallHook()
    {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_ReadyWeaponHandler[0]);
        func = vTable.write_vfunc(0x4, &ReadyWeaponHandlerHook::thunk);
    }

    void ReadyWeaponHandlerHook::thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
    {
		if (!a_this || !a_event || !a_data) return func(a_this, a_event, a_data);

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player->AsActorState()->IsWeaponDrawn()) {
			return func(a_this, a_event, a_data);
        }

		if (!a_event->IsUp()) return;
        if (a_event->HeldDuration() < held_threshold) {
			a_event->value = 1.0f;
			a_event->heldDownSecs = 0.f;
            return func(a_this, a_event, a_data);
        }

		const auto weapon_L = player->GetEquippedObject(true);
		RE::TESForm* weapon = weapon_L ? weapon_L : player->GetEquippedObject(false);
		if (!weapon || !weapon->IsWeapon()) return func(a_this, a_event, a_data);

		SKSE::GetTaskInterface()->AddTask([player, weapon]() {
			const auto bound = weapon->As<RE::TESBoundObject>();
            RE::ActorEquipManager::GetSingleton()->UnequipObject(player,bound);
            player->RemoveItem(bound,1,RE::ITEM_REMOVE_REASON::kDropping,nullptr,nullptr);
		});

		return func(a_this, a_event, a_data);
    }

    
    void EquipObjectOverRide::InstallHook(SKSE::Trampoline& a_trampoline) {
        const REL::Relocation<std::uintptr_t> target{RELOCATION_ID(37938, 38894), REL::VariantOffset(0xE5, 0x170, 0xE5)};
        EquipObjectOverRide::func = a_trampoline.write_call<5>(target.address(), EquipObjectOverRide::thunk);
    }

    void EquipObjectOverRide::thunk(RE::ActorEquipManager* a_self, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                    std::uint64_t a_unk) {

        if (!a_object || !a_actor || !a_self) {
            func(a_self, a_actor, a_object, a_unk);
            return;
        }


        if (a_object->IsWeapon()) {  // It's a weapon
            if (const RE::ActorState* actorState = a_actor->AsActorState()) {                       // nullptr check
                /*if (actorState->GetWeaponState()==RE::WEAPON_STATE::kWantToDraw ||
                    actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawing) {
					return;
                }*/
                if (actorState->IsWeaponDrawn()) {  // use kDrawn?
                    
                    RE::TESForm* RequippedObject = a_actor->GetEquippedObject(false);
                    RE::TESForm* LequippedObject = a_actor->GetEquippedObject(true);

                    if (RequippedObject && RequippedObject->Is(RE::FormType::Weapon)) {
                        a_actor->DrawWeaponMagicHands(false);
						std::unique_lock ulock(actor_queue_mutex);
						const auto actor_info = ActorInfo(a_actor, a_object);
						if (const auto it = actor_queue.find(actor_info); it != actor_queue.end()) {
							actor_queue.erase(it);
						}
						actor_queue.insert(actor_info);
#ifndef NDEBUG
						logger::trace("R Weapon equipped, adding to queue {}", a_actor->GetName());
#endif
                        return;
                    }
                    if (LequippedObject) {
						if (LequippedObject->Is(RE::FormType::Weapon)) {
							a_actor->DrawWeaponMagicHands(false);
							std::unique_lock ulock(actor_queue_mutex);
                            const auto actor_info = ActorInfo(a_actor, a_object);
							if (const auto it = actor_queue.find(actor_info); it != actor_queue.end()) {
								actor_queue.erase(it);
							}
							actor_queue.insert(actor_info);
#ifndef NDEBUG
							logger::trace("L Weapon equipped, adding to queue {}", a_actor->GetName());
#endif
							return;
                        }
						if (LequippedObject->Is(RE::FormType::Armor)) {  // Shield
							a_actor->DrawWeaponMagicHands(false);
							std::unique_lock ulock(actor_queue_mutex);
							const auto actor_info = ActorInfo(a_actor, a_object);
							if (const auto it = actor_queue.find(actor_info); it != actor_queue.end()) {
								actor_queue.erase(it);
							}
							actor_queue.insert(actor_info);
#ifndef NDEBUG
							logger::trace("Shield equipped, adding to queue {}", a_actor->GetName());
#endif
							return;
                        }
                    }
                }
            }
        }
        func(a_self, a_actor, a_object, a_unk);
    }

    void Install()
    {
        constexpr size_t size_per_hook = 14;
        auto& trampoline = SKSE::GetTrampoline();
        SKSE::AllocTrampoline(size_per_hook*1);

		ReadyWeaponHandlerHook::InstallHook();
        EquipObjectOverRide::InstallHook(trampoline);
		ActorUpdateHook<RE::Character>::InstallHook();
		ActorUpdateHook<RE::PlayerCharacter>::InstallHook();
		UnEquipHook<RE::Character>::InstallHook();

    }

    template <class T>
	void ActorUpdateHook<T>::InstallHook() {
		REL::Relocation<std::uintptr_t> vTable(T::VTABLE[0]);
		func = vTable.write_vfunc(0xAD, &ActorUpdateHook::thunk);
    }

	template <class T>
	void ActorUpdateHook<T>::thunk(T* a_this, float a_delta) {
		if (!a_this) return func(a_this, a_delta);

        if (a_this->AsActorState()->IsWeaponDrawn()) {
			return func(a_this, a_delta);
        }

        if (std::shared_lock lock(actor_queue_mutex); actor_queue.empty()) {
		    return func(a_this, a_delta);
        }
        else if (const auto dummy_actor_info = ActorInfo(a_this); !actor_queue.contains(dummy_actor_info)) {
		    return func(a_this, a_delta);
        }
		else {
			const auto it = actor_queue.find(dummy_actor_info);
            lock.unlock();
		    RE::ActorEquipManager::GetSingleton()->EquipObject(a_this, it->queued_weap);
            a_this->DrawWeaponMagicHands(true);
			std::unique_lock ulock(actor_queue_mutex);
#ifndef NDEBUG
			logger::trace("Erasing from queue: {}", a_this->GetName());
#endif
			actor_queue.erase(it);
        }

		func(a_this, a_delta);
    }
};