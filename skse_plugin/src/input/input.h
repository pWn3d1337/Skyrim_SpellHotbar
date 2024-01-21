#pragma once

namespace SpellHotbar::Input {

	void install_hook();

	void processAndFilter(RE::InputEvent** a_event);

	class KeyModifier
	{
    public:
        KeyModifier(uint8_t code1, uint8_t code2 = 0);

		void update(RE::ButtonEvent* bEvent);

		inline bool isDown();
    private:
        uint8_t keycode;
        uint8_t keycode2;

		bool isDown1;
        bool isDown2;
	};

	inline bool KeyModifier::isDown()
	{
		return isDown1 || isDown2;
	}

	extern KeyModifier mod_ctrl;
    extern KeyModifier mod_shift;
    extern KeyModifier mod_alt;
}