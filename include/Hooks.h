#pragma once
#include <shared_mutex>


namespace Hooks {

	void Install();

	inline constexpr float held_threshold = 0.24f;

	struct ReadyWeaponHandlerHook {
        static void InstallHook();
	    static void thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, [[maybe_unused]] RE::PlayerControlsData* a_data);
		static inline REL::Relocation<decltype(thunk)> func;

	};

	struct EquipObjectOverRide {
        static void InstallHook(SKSE::Trampoline& a_trampoline);
        static void thunk(RE::ActorEquipManager* a_self, RE::Actor* a_actor, RE::TESBoundObject* a_object, std::uint64_t a_unk);
        static inline REL::Relocation<decltype(thunk)> func;
    };

	template <class T>
	struct UnEquipHook {
		static void InstallHook();
		static void thunk(T* a_this, std::uint64_t a_arg1, RE::TESBoundObject* a_object);
		static inline REL::Relocation<decltype(thunk)> func;
	};

	template <class T>
	struct ActorUpdateHook {
		static void InstallHook();
		static void thunk(T* a_this, float a_delta);
		static inline REL::Relocation<decltype(thunk)> func;
	};

    struct ActorInfo {
	    FormID baseID;
		RefID refID;
		RE::TESBoundObject* queued_weap=nullptr;

		bool operator<(const ActorInfo& rhs) const
        {
            return baseID < rhs.baseID ||
                   (baseID == rhs.baseID && refID < rhs.refID);
        }

		bool operator ==(const ActorInfo& rhs) const {
			return baseID == rhs.baseID && refID == rhs.refID;
		}

        explicit ActorInfo(RE::Actor* a_this) {
			baseID = a_this->GetBaseObject()->GetFormID();
			refID = a_this->GetFormID();
		}

		explicit ActorInfo(RE::Actor* a_this,RE::TESBoundObject* a_weap) {
			baseID = a_this->GetBaseObject()->GetFormID();
			refID = a_this->GetFormID();
			queued_weap = a_weap;
		}
	};

	inline std::shared_mutex actor_queue_mutex;
	inline std::set<ActorInfo> actor_queue;

}