#include "hotbar.h"
#include "../logger/logger.h"
#include "../storage/storage.h"
#include "hotbars.h"
#include "../game_data/game_data.h"
#include "../rendering/render_manager.h"


namespace SpellHotbar
{
    namespace rj = rapidjson;

    Hotbar::Hotbar(const std::string & name)
        : m_enabled(true), 
          m_bar(),
          m_ctrl_bar(),
          m_shift_bar(),
          m_alt_bar(),
          m_parent_bar(0),
          m_name(name),
          m_inherit_mode(inherit_mode::def)
    {
    }
    void Hotbar::set_parent(uint32_t parent)
    {
        m_parent_bar = parent;
    }

    void serialize_bar(const SubBar& bar, SKSE::SerializationInterface* serializer, const std::string & name) {

        // write number off filled slots
        uint8_t filled_slots = static_cast<uint8_t>(std::count_if(bar.m_slotted_skills.begin(), bar.m_slotted_skills.end(), [](const auto& elem) { return elem.formID != 0; }));
        if (!serializer->WriteRecordData(&filled_slots, sizeof(uint8_t))) {
            logger::error("Failed to write bar size for {}", name);
            return;
        }

        // write index + formID
        for (int i = 0U; i < bar.m_slotted_skills.size(); i++) {
            if (bar.m_slotted_skills.at(i).formID != 0) {
                uint8_t index = static_cast<uint8_t>(i);
                if (!serializer->WriteRecordData(&index, sizeof(uint8_t))) {
                    logger::error("Failed to write data for {}_{}", name, i);
                    break;
                }
                if (!serializer->WriteRecordData(&bar.m_slotted_skills[i].formID, sizeof(RE::FormID))) {
                    logger::error("Failed to write data for {}_{}", name, i);
                    break;
                }
            }
        }

    }

    void deserialize_bar(SubBar& bar, SKSE::SerializationInterface* serializer, const std::string& name, uint32_t /*version*/, uint32_t /*length*/)
    {       
        //read number of filled slots
        uint8_t slots{0Ui8};
        if (!serializer->ReadRecordData(&slots, sizeof(uint8_t))) {
            logger::error("Failed to read slots count for Hotbar {}!", name);
        }

        for (uint8_t i = 0U; i < slots; i++) {
            uint8_t read_slot{0Ui8};
            RE::FormID read_id{0U};

            if (!serializer->ReadRecordData(&read_slot, sizeof(uint8_t))) {
                logger::error("Failed to load Hotbar {}!", name);
                break;
            } else {
                read_slot = std::clamp(read_slot, 0Ui8, static_cast<uint8_t>(max_bar_size));
            }

            if (!serializer->ReadRecordData(&read_id, sizeof(RE::FormID))) {
                logger::error("Failed to load Hotbar {}!", name);
                break;
            } else {
                RE::FormID resolved_id{0};
                serializer->ResolveFormID(read_id, resolved_id);
                bar.m_slotted_skills[read_slot] = resolved_id;
            }
        }

    }

    void Hotbar::serialize(SKSE::SerializationInterface* serializer, uint32_t key) const
    {
        if (!serializer->OpenRecord(key, Storage::save_format)) {
            if (Bars::bar_names.contains(key)) {
                logger::error("Failed to open record for {}!", Bars::bar_names.at(key));
            } else {
                logger::error("No barname known for {}!", key);
            }
        } else {
            //if (Bars::bar_names.contains(key)) logger::trace("Storing bar {}", Bars::bar_names.at(key));
            if (!serializer->WriteRecordData(&m_enabled, sizeof(bool))) {
                logger::error("Failed to write enabled state for {}", m_name);
            } else {
                // write inherit_mode
                uint8_t inherit_type = static_cast<uint8_t>(m_inherit_mode);
                if (!serializer->WriteRecordData(&inherit_type, sizeof(uint8_t))) {
                    logger::error("Failed to write data inherit mode for {}", Bars::bar_names.at(key));
                }

                serialize_bar(m_bar, serializer, m_name + "_main");
                serialize_bar(m_ctrl_bar, serializer, m_name + "_ctrl");
                serialize_bar(m_shift_bar, serializer, m_name + "_shift");
                serialize_bar(m_alt_bar, serializer, m_name + "_alt");
            }
        }
    }

    void Hotbar::deserialize(SKSE::SerializationInterface* serializer, uint32_t type, uint32_t /*version*/,
                             uint32_t length) {
        // only 1 version for now, we can ignore version variable

        std::string name = Bars::bar_names.contains(type) ? Bars::bar_names.at(type) : "?";
        //logger::trace("Reading hotbar {} from save...", name);

        // bar record length can vary, no length check
        if (!serializer->ReadRecordData(&m_enabled, sizeof(bool))) {
            logger::error("Failed to read enabled state for Hotbar {}!", name);
        } else {

            uint8_t inherit_type{0};
            if (!serializer->ReadRecordData(&inherit_type, sizeof(uint8_t))) {
                logger::error("Failed to read inherit type for Hotbar {}!", name);
            } else {
                m_inherit_mode = inherit_mode(static_cast<int>(inherit_type));
            }

            deserialize_bar(m_bar, serializer, name, type, length);
            deserialize_bar(m_ctrl_bar, serializer, name, type, length);
            deserialize_bar(m_shift_bar, serializer, name, type, length);
            deserialize_bar(m_alt_bar, serializer, name, type, length);
        }

        if (type == Bars::MAIN_BAR) {
            m_enabled = true; // Mainbar MUST be enabled
            m_inherit_mode = inherit_mode::none; // Mainbar can't inherit
        }
        /* uint32_t num_bars = 1;
        num_bars += (Bars::enable_ctrl_bars ? 1U : 0U);
        num_bars += (Bars::enable_shift_bars ? 1U : 0U);
        num_bars += (Bars::enable_alt_bars ? 1U : 0U);
        uint32_t expected_length = (max_bar_size * sizeof(RE::FormID) + sizeof(uint8_t)) * num_bars;

        if (length != expected_length) {
            logger::error("Invalid Length ({}) for Hotbar {}, expected {}", length, type, expected_length);
        } else {

            deserialize_bar(m_bar, serializer, name, type, length);
        }*/
    }

    SubBar& Hotbar::get_sub_bar(key_modifier mod)
    {
        switch (mod) {
            case SpellHotbar::key_modifier::ctrl:
                return m_ctrl_bar;
            case SpellHotbar::key_modifier::shift:
                return m_shift_bar;
            case SpellHotbar::key_modifier::alt:
                return m_alt_bar;
            default:
                return m_bar;
        }
    }

    std::tuple<RE::FormID, slot_type, bool> Hotbar::get_skill_in_bar_with_inheritance(
        int index, key_modifier mod, bool hide_clear_spell, bool inherited, std::optional<key_modifier> original_mod)
    {
        auto& bar = get_sub_bar(mod).m_slotted_skills[index];
        RE::FormID skill = bar.formID;
        slot_type type = bar.type;

        if (skill != 0U && this->is_enabled())
        {
            if (hide_clear_spell && GameData::is_clear_spell(skill)) {
                return std::make_tuple(0U, slot_type::empty, inherited);
            } else {
                return std::make_tuple(skill, type, inherited);
            }
        } else {
            if (m_parent_bar != 0U && Bars::hotbars.contains(m_parent_bar) && (m_inherit_mode != inherit_mode::none)) {
                //logger::info("Asking Parent Bar: {} {}", m_parent_bar.get()->get_name(), (int) &(*m_parent_bar));
                if (mod != key_modifier::none && m_inherit_mode == inherit_mode::def) {
                    //if we have a modifier we need inherit from non-mod bar
                    return get_skill_in_bar_with_inheritance(index, key_modifier::none, hide_clear_spell, true, mod);
                }

                return Bars::hotbars.at(m_parent_bar).get_skill_in_bar_with_inheritance(index, original_mod ? original_mod.value() : mod, hide_clear_spell, true);
            } else {
                return std::make_tuple(0U, slot_type::empty, false);
            }
        }
    }

    int Hotbar::set_inherit_mode(int value)
    {
        int mode = std::clamp(value, 0, 2);
        m_inherit_mode = static_cast<inherit_mode>(mode);
        return mode;
    }

    void Hotbar::draw_in_menu(ImFont* font, float /* screensize_x*/, float screensize_y, int highlight_slot,
                              float highlight_factor, key_modifier mod)
    {
        ImGui::PushFont(font);

        int icon_size = static_cast<int>(get_slot_height(screensize_y));
        float text_offset_x = icon_size * 0.05f;
        float text_offset_y = icon_size * 0.0125f;
        //float text_height = ImGui::CalcTextSize("M").y;

        for (int i = 0; i < Bars::barsize; i++) {
            auto [skill_id, skill_type, inherited] = get_skill_in_bar_with_inheritance(i, mod, false);

            ImVec2 p = ImGui::GetCursorScreenPos();

            if (!RenderManager::draw_skill(skill_id, icon_size)) {
                RenderManager::draw_bg(icon_size);
            } else {
                ImU32 col = IM_COL32_WHITE;
                if (highlight_slot == i) {
                    col = IM_COL32(255, 255, static_cast<int>(127 + 128 * (1.0 - highlight_factor)), 255);
                }

                RenderManager::draw_slot_overlay(p, icon_size, col);
            }
            ImGui::SameLine();

            std::string key_text = GameData::get_keybind_text(i, mod);
            //ImVec2 tex_pos(p.x + text_offset,
              //             p.y + (static_cast<float>(icon_size) * Bars::slot_scale) - text_height - text_offset);
            ImVec2 tex_pos(p.x + text_offset_x, p.y+text_offset_y);
            ImGui::GetWindowDrawList()->AddText(tex_pos, ImColor(255, 255, 255), key_text.c_str());



            std::string text = GameData::resolve_spellname(skill_id);

            //draw text in grey if it was inherited from parent bar
            int grey_val = inherited ? 127 : 255;
            auto color = ImColor(grey_val, grey_val, grey_val);

            if (highlight_slot == i) {
                color = ImColor(255, 255, 127 + static_cast<int>(128.0 * (1.0f - highlight_factor)));
            }

            ImGui::TextColored(color, text.c_str());
        }
        ImGui::PopFont();
    }

    inline float determine_cd(RE::FormID skill, slot_type skill_type, float game_time, float time_scale, float gcd_prog,
                              float gcd_dur, float shout_cd, float shout_cd_dur) {
        float cd_prog {0.0f};

        constexpr float gt_to_sec_factor = 24.0f * 60.0f * 60.0f;

        auto [gt_prog, gt_dur] = GameData::get_gametime_cooldown(game_time, skill);
        if (gt_dur > 0.0f)
        {
            // get longer CD
            float gcd = gcd_dur * (1.0f - gcd_prog);
            float gt_cd = gt_dur * gt_to_sec_factor / time_scale * (1.0f - gt_prog);

            cd_prog = (gt_cd >= gcd) ? gt_prog : gcd_prog;

        } else {
            if (skill_type == slot_type::shout && shout_cd_dur > 0.0f) {
                // get longer cd
                float gcd = gcd_dur * (1.0f - gcd_prog);
                float scd = shout_cd_dur * (1.0f - shout_cd);
                cd_prog = (scd >= gcd) ? shout_cd : gcd_prog;
            } else {
                if (gcd_prog > 0.0f) {
                    cd_prog = gcd_prog;
                }
            }
        }
        return cd_prog;
    }

    void Hotbar::draw_in_hud(ImFont* font, float /* screensize_x*/, float screensize_y, int highlight_slot,
                             float highlight_factor, key_modifier mod, bool highlight_isred, float alpha, float shout_cd, float shout_cd_dur) {
        ImGui::PushFont(font);

        int icon_size = static_cast<int>(get_hud_slot_height(screensize_y));
        float text_offset_x = icon_size * 0.05f;
        float text_offset_y = icon_size * 0.0125f;

        float gcd_prog = GameData::get_current_gcd_progress();
        float gcd_dur = GameData::get_current_gcd_duration();

        float game_time{0};
        float time_scale{20.0f};
        RE::Calendar* cal = RE::Calendar::GetSingleton();
        if (cal) {
            game_time = cal->GetCurrentGameTime();
            time_scale = cal->GetTimescale();
        }
        auto pc = RE::PlayerCharacter::GetSingleton();

        for (int i = 0; i < Bars::barsize; i++) {
            auto [skill_id, skill_type, inherited] = get_skill_in_bar_with_inheritance(i, mod, true);

            ImVec2 p = ImGui::GetCursorScreenPos();

            int alpha_i = static_cast<int>(255 * alpha);
            if (!RenderManager::draw_skill(skill_id, icon_size, alpha)) {
                RenderManager::draw_bg(icon_size, alpha);
            } else {

                //If vampire lord uses equipmode and this is the currently equipped spell -> highlight blue
                if (this->get_name() == Bars::bar_names.at(Bars::VAMPIRE_LORD_BAR) &&
                    GameData::global_vampire_lord_equip_mode && GameData::global_vampire_lord_equip_mode->value > 0.0f) {
                    if (pc) {
                        auto equipped_mh = pc->GetEquippedObject(false);
                        //If mainhand not empty, we in cast mode
                        if (equipped_mh) {
                            auto equipped_oh = pc->GetEquippedObject(true);
                            if (equipped_oh && equipped_oh->GetFormID() == skill_id) {
                                RenderManager::draw_highlight_overlay(p, icon_size, IM_COL32(127, 127, 255, alpha_i));
                            }
                        } else {
                            //empty mh -> melee mode
                            if (skill_type == slot_type::spell) {
                                RenderManager::draw_cd_overlay(p, icon_size, 0.0f, IM_COL32(255, 255, 255, alpha_i));
                            }
                        }
                    }
                }

                float cd_prog =
                    determine_cd(skill_id, skill_type, game_time, time_scale, gcd_prog, gcd_dur, shout_cd, shout_cd_dur);
                if (cd_prog > 0.0f) {
                    RenderManager::draw_cd_overlay(p, icon_size, cd_prog, IM_COL32(255,255,255, alpha_i));
                }
                RenderManager::draw_slot_overlay(p, icon_size, IM_COL32(255,255,255, alpha_i));
            }
            if (highlight_slot == i) {
                ImU32 col;
                if (highlight_isred) {
                    int f = static_cast<int>(255 * (highlight_factor)*alpha);
                    col = IM_COL32(255, 0, 0, f);
                } else {
                    col = IM_COL32(255, 255, static_cast<int>(255 * (1.0f - highlight_factor)), alpha_i);
                    // static_cast<int>(255 * (1.0f - highlight_factor)));
                }
                RenderManager::draw_highlight_overlay(p, icon_size, col);
            }

            ImGui::SameLine();

            std::string key_text = GameData::get_keybind_text(i, mod);
            //ImVec2 tex_pos(p.x + text_offset, p.y + (static_cast<float>(icon_size) * Bars::slot_scale) - text_height - text_offset);
            ImVec2 tex_pos(p.x + text_offset_x, p.y + text_offset_y);
            RenderManager::draw_scaled_text(tex_pos, ImColor(255, 255, 255, alpha_i), key_text.c_str());

            ImGui::SameLine();
        }
        ImGui::PopFont();
    }

    void SpellHotbar::Hotbar::slot_spell(size_t index, RE::FormID spell, key_modifier modifier)
    {
        if (GameData::is_clear_spell(spell)) {
            if (get_spell(index, modifier) > 0U) {
                spell = 0U;
            }
        }

        if (index < max_bar_size) {
            switch (modifier) {
                case key_modifier::ctrl:
                    m_ctrl_bar.m_slotted_skills[index] = spell;
                    break;
                case key_modifier::shift:
                    m_shift_bar.m_slotted_skills[index] = spell;
                    break;
                case key_modifier::alt:
                    m_alt_bar.m_slotted_skills[index] = spell;
                    break;
                case key_modifier::none:
                default:
                    m_bar.m_slotted_skills[index] = spell;
                    break;
            }
        }
    }
    RE::FormID Hotbar::get_spell(size_t index, key_modifier modifier)
    {
        RE::FormID ret{0};
        if (index < max_bar_size) {
            switch (modifier) {
                case key_modifier::ctrl:
                    ret = m_ctrl_bar.m_slotted_skills[index].formID;
                    break;
                case key_modifier::shift:
                    ret = m_shift_bar.m_slotted_skills[index].formID;
                    break;
                case key_modifier::alt:
                    ret = m_alt_bar.m_slotted_skills[index].formID;
                    break;
                case key_modifier::none:
                default:
                    ret = m_bar.m_slotted_skills[index].formID;
                    break;
            }
        }
        return ret;
    }

    SlottedSkill::SlottedSkill() : SlottedSkill(0U){};

    SlottedSkill::SlottedSkill(RE::FormID id) : formID(id), type(slot_type::empty)
    { 
        if (formID > 0U)
        {
            if (GameData::is_clear_spell(formID)) {
                type = slot_type::blocked;
            } else {
                auto form = RE::TESForm::LookupByID(formID);
                RE::SpellItem* spell{nullptr};
                if (form) {
                    switch (form->GetFormType()) {
                        case RE::FormType::Spell:
                            spell = form->As<RE::SpellItem>();
                            if (spell) {  // this should not be able to be null
                                if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
                                    type = slot_type::lesser_power;
                                } else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower) {
                                    type = slot_type::power;
                                } else {
                                    type = slot_type::spell;
                                }
                            } else {
                                type = slot_type::unknown;
                            }
                            break;
                        case RE::FormType::Shout:
                            type = slot_type::shout;
                            break;
                        default:
                            type = slot_type::unknown;
                            break;
                    }
                } else {
                    type = slot_type::unknown;
                }
            }
        }
    }

    bool SlottedSkill::isEmpty() {
        return type==slot_type::empty;
    }

    void SlottedSkill::clear()
    { 
        type = slot_type::empty;
        formID = 0U;
    }

    bool SubBar::is_empty()
    { 
        return !std::any_of(m_slotted_skills.begin(), m_slotted_skills.end(), [](auto& elem) { return !elem.isEmpty(); });
    }

    void SubBar::clear() {
        for (auto& slot : m_slotted_skills) {
            slot.clear();
        }
    }

    void subbar_to_json(rj::Document& doc, SubBar& bar, rj::Value& bar_node, key_modifier mod)
    {
        rj::Value bar_array(rj::kArrayType);

        for (size_t i = 0U; i < bar.m_slotted_skills.size(); i++) {
            if (!bar.m_slotted_skills[i].isEmpty()) {
                RE::FormID id = bar.m_slotted_skills[i].formID;
                auto form = RE::TESForm::LookupByID(id);
                if (form) {
                    auto file = form->GetFile(0);
                    if (file) {
                        const std::string_view& name = file->GetFilename();
                        RE::FormID localid = form->GetLocalFormID();

                        rj::Value slot_data(rj::kObjectType);
                        slot_data.AddMember("index", i, doc.GetAllocator());
                        slot_data.AddMember("form", localid, doc.GetAllocator());
                        rj::Value fn(name.data(), doc.GetAllocator());
                        slot_data.AddMember("file", fn, doc.GetAllocator());

                        bar_array.PushBack(slot_data, doc.GetAllocator());
                    } else {
                        logger::error("Could not save {}, no origin file found", id);
                    }
                } else {
                    logger::error("Could not save {}, form not found", id);
                }


            }
        }

        std::string bar_mod;
        switch (mod) {
            case key_modifier::ctrl:
                bar_mod = "ctrl";
                break;
            case key_modifier::shift:
                bar_mod = "shift";
                break;
            case key_modifier::alt:
                bar_mod = "alt";
                break;
            case key_modifier::none:
            default:
                bar_mod = "none";
                break;
        }

        rj::Value tag(bar_mod.c_str(), doc.GetAllocator());

        bar_node.AddMember(tag, bar_array, doc.GetAllocator());
    }

    void Hotbar::to_json(rj::Document& doc, uint32_t key, rj::Value& bars)
    { 
        rj::Value bar_node(rj::kObjectType);
        bar_node.AddMember("id", key, doc.GetAllocator());
        bool add{false};
        if (!m_bar.is_empty())
        {
            subbar_to_json(doc, m_bar, bar_node, key_modifier::none);
            add = true;
        }
        if (!m_ctrl_bar.is_empty()) {
            subbar_to_json(doc, m_ctrl_bar, bar_node, key_modifier::ctrl);
            add = true;
        }
        if (!m_shift_bar.is_empty()) {
            subbar_to_json(doc, m_shift_bar, bar_node, key_modifier::shift);
            add = true;
        }
        if (!m_alt_bar.is_empty()) {
            subbar_to_json(doc, m_alt_bar, bar_node, key_modifier::alt);
            add = true;
        }

        if (add) {
            bars.PushBack(bar_node, doc.GetAllocator());
        }
    }

    void Hotbar::clear() {
        m_bar.clear();
        m_ctrl_bar.clear();
        m_shift_bar.clear();
        m_alt_bar.clear();
    }
}