#pragma once
namespace SpellHotbar::events {
    class EventListener : public RE::BSTEventSink<RE::TESSpellCastEvent>
    {
    public:

        static EventListener* GetSingleton();

        EventListener(const EventListener&) = delete;
        EventListener& operator=(const EventListener) = delete;
    
        RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event,
                                              RE::BSTEventSource<RE::TESSpellCastEvent>*);

    private:
        EventListener() = default;

    };
}