#include "input.h"
#include "../logger/logger.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "../rendering/render_manager.h"

namespace {

    // from https://github.com/SlavicPotato/ied-dev / https://github.com/D7ry/wheeler/

    /// Hooks the input event dispatching function, this function dispatches a linked list of input events
    /// to other input event handlers, hence by modifying the linked list we can filter out unwanted input events.
    class OnInputEventDispatch {
    public:
        static void Install() {
            auto& trampoline = SKSE::GetTrampoline();
            REL::Relocation<uintptr_t> caller{RELOCATION_ID(67315, 68617)};
            _DispatchInputEvent = trampoline.write_call<5>(
                caller.address() + REL::VariantOffset(0x7B, 0x7B, 0).offset(), DispatchInputEvent);
        }

    private:
        static void DispatchInputEvent(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent** a_evns) {
            if (!a_evns) {
                _DispatchInputEvent(a_dispatcher, a_evns);
                return;
            }

            SpellHotbar::Input::processAndFilter(a_evns);

            _DispatchInputEvent(a_dispatcher, a_evns);
        }
        static inline REL::Relocation<decltype(DispatchInputEvent)> _DispatchInputEvent;
    };


}
namespace SpellHotbar::Input {

    void install_hook() { OnInputEventDispatch::Install(); }

    void processAndFilter(RE::InputEvent** a_event)
    {
        //mostly taken from wheeler
        if (!a_event) {
            return;
        }

        RE::InputEvent* event = *a_event;
        RE::InputEvent* prev = nullptr;
        while (event != nullptr) {
            bool captureEvent = false; //Capure this event? (not forward to game)
            auto eventType = event->eventType;

            if (event->eventType == RE::INPUT_EVENT_TYPE::kMouseMove) {
                if (RenderManager::should_block_game_cursor_inputs()) {
                    //RE::MouseMoveEvent* mouseMove = static_cast<RE::MouseMoveEvent*>(event);
                    captureEvent = true;
                }
            } else if (event->eventType == RE::INPUT_EVENT_TYPE::kThumbstick) {
                if (RenderManager::should_block_game_cursor_inputs()) {
                    RE::ThumbstickEvent* thumbstick = static_cast<RE::ThumbstickEvent*>(event);
                    if (thumbstick->IsRight()) {
                        captureEvent = true;
                    }
                }
            } else if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
                RE::ButtonEvent* bEvent = event->AsButtonEvent();
                if (bEvent) {
                    auto& io = ImGui::GetIO();
                    mod_ctrl.update(bEvent);
                    mod_shift.update(bEvent);
                    mod_alt.update(bEvent);

                    if (bEvent->device == RE::INPUT_DEVICE::kMouse) {
                        // credits open animation replacer
                        //forward the mouse inputs to ImGUI
                        if (bEvent->GetIDCode() > 7) {
                            io.AddMouseWheelEvent(0, bEvent->value * (bEvent->GetIDCode() == 8 ? 1 : -1));
                        } else {
                            if (bEvent->GetIDCode() > 5) bEvent->idCode = 5;
                            io.AddMouseButtonEvent(bEvent->idCode, bEvent->IsPressed());
                        }
                        if (RenderManager::should_block_game_cursor_inputs()) {
                            captureEvent = true;
                        }
                    }
                    else if (bEvent->device == RE::INPUT_DEVICE::kKeyboard) {
                        if (RenderManager::should_block_game_cursor_inputs() && bEvent->GetIDCode() == 1) {  // ESC button
                            RenderManager::stop_bar_dragging();
                            captureEvent = true;
                        }
                    }
                }
            }

            RE::InputEvent* nextEvent = event->next;
            if (captureEvent) {
                //remove event out of queue
                if (prev != nullptr) {
                    prev->next = nextEvent;
                } else {
                    *a_event = nextEvent;
                }
            } else {
                prev = event;
            }
            event = nextEvent;
        }
    }

    KeyModifier mod_ctrl(29, 157);
    KeyModifier mod_shift(42, 54);
    KeyModifier mod_alt(56, 184);

    KeyModifier::KeyModifier(uint8_t code1, uint8_t code2)
        : keycode(code1), keycode2(code2), isDown1(false), isDown2(false)
    {}

    void KeyModifier::update(RE::ButtonEvent* bEvent) {
        if (bEvent->idCode == keycode)
        {
            isDown1 = bEvent->IsPressed();
        }
        else if (bEvent->idCode == keycode2)
        {
            isDown2 = bEvent->IsPressed();
        }
    }
}
