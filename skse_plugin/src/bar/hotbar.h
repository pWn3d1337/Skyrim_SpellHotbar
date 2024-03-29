#pragma once
#include <memory>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <rapidjson/document.h>

namespace SpellHotbar
{

constexpr size_t max_bar_size = 12U;

enum class slot_type: uint8_t {
    empty = 0,
    unknown,
    blocked,
    spell,
    power,
    lesser_power,
    shout
};

enum inherit_mode
{
    def=0, //default, inherit from non-mod bar (e.G Shift+Melee inherits from Melee)
    same_modifier=1, //inherit from parent with same mod (e.G Shift+Melee inherits from Shift+Main)
    none=2 //do not inherit any slots at all
};

struct SlottedSkill
{
    RE::FormID formID;
    slot_type type;

    SlottedSkill(RE::FormID id);
    SlottedSkill();
    void clear();
    bool isEmpty();
};

struct SubBar {
    std::array<SlottedSkill, max_bar_size> m_slotted_skills;

    SubBar()
    {
        m_slotted_skills.fill(0U);
    }

    bool is_empty();
    void clear();
};

enum class key_modifier
{ 
    none,
    ctrl,
    shift,
    alt
};


class Hotbar
{
public:
    Hotbar(const std::string & name);

    void set_parent(uint32_t parent);

    inline uint32_t get_parent() const;

    void serialize(SKSE::SerializationInterface* serializer, uint32_t key) const;
    void deserialize(SKSE::SerializationInterface* serializer, uint32_t type, uint32_t version, uint32_t length);

    inline void set_enabled(bool enabled);

    void draw_in_menu(ImFont* font, float screensize_x, float screensize_y, int highlight_slot, float highlight_factor, key_modifier mod);

    void draw_in_hud(ImFont* font, float screensize_x, float screensize_y, int highlight_slot, float highlight_factor, key_modifier mod,
        bool hightlight_isred, float alpha, float shout_cd, float shout_cd_dur);

    inline bool is_enabled() const;

    inline const std::string& get_name() const;

    void slot_spell(size_t index, RE::FormID spell, key_modifier modifier);

    RE::FormID get_spell(size_t index, key_modifier modifier);

    inline int get_inherit_mode() const;

    int set_inherit_mode(int value);

    /* return skillid and if it was inherited */
    std::tuple<RE::FormID, slot_type, bool> get_skill_in_bar_with_inheritance(
        int index, key_modifier mod, bool hide_clear_spell, bool inherited = false, std::optional<key_modifier> original_mod = std::nullopt);

    void to_json(rapidjson::Document& doc, uint32_t key, rapidjson::Value& bars);

    void clear();

    SubBar& get_sub_bar(key_modifier mod);

private:
    bool m_enabled;
    SubBar m_bar;
    SubBar m_ctrl_bar;
    SubBar m_shift_bar;
    SubBar m_alt_bar;

    uint32_t m_parent_bar;
    std::string m_name;
    inherit_mode m_inherit_mode;
};

inline void SpellHotbar::Hotbar::set_enabled(bool enabled) { m_enabled = enabled; }
inline bool SpellHotbar::Hotbar::is_enabled() const { return m_enabled; }
inline const std::string& SpellHotbar::Hotbar::get_name() const { return m_name; }
inline int SpellHotbar::Hotbar::get_inherit_mode() const { return static_cast<int>(m_inherit_mode); }
inline uint32_t SpellHotbar::Hotbar::get_parent() const { return m_parent_bar; };
}