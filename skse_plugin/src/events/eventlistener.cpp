#include "eventlistener.h"
#include "../logger/logger.h"
#include "../game_data/game_data.h"

namespace SpellHotbar::events {

    EventListener* EventListener::GetSingleton() {
        static EventListener instance;
        return &instance;
    }

    RE::BSEventNotifyControl EventListener::ProcessEvent(const RE::TESSpellCastEvent* event,
                                                         RE::BSTEventSource<RE::TESSpellCastEvent>*) {
        if (event) {
            if (event->object && (event->object->IsPlayerRef() || event->object->IsPlayer()))
            {
                auto form = RE::TESForm::LookupByID(event->spell);
                if (form && form->GetFormType() == RE::FormType::Spell) {
                    RE::SpellItem* spell = form->As<RE::SpellItem>();
                    if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kPower)
                    {
                        GameData::add_gametime_cooldown(event->spell, 24.0);
                    }
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }


}