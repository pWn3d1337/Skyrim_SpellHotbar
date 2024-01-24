#pragma once
#include "../bar/hotbar.h"
#include "../logger/logger.h"

namespace SpellHotbar::GameData {
    enum class EquippedType
    {
        FIST,
        ONEHAND_EMPTY,
        ONEHAND_SHIELD,
        ONEHAND_SPELL,
        DUAL_WIELD,
        TWOHAND,
        RANGED,
        SPELL
    };

    enum class DefaultIconType : uint32_t {
        UNKNOWN = 0U,
        BAR_EMPTY,
        BAR_OVERLAY,
        BAR_HIGHLIGHT,
        UNBIND_SLOT,
        LESSER_POWER,
        GREATER_POWER,
        DESTRUCTION_FIRE_NOVICE,
        DESTRUCTION_FIRE_APPRENTICE,
        DESTRUCTION_FIRE_ADEPT,
        DESTRUCTION_FIRE_EXPERT,
        DESTRUCTION_FIRE_MASTER,
        DESTRUCTION_FROST_NOVICE,
        DESTRUCTION_FROST_APPRENTICE,
        DESTRUCTION_FROST_ADEPT,
        DESTRUCTION_FROST_EXPERT,
        DESTRUCTION_FROST_MASTER,
        DESTRUCTION_SHOCK_NOVICE,
        DESTRUCTION_SHOCK_APPRENTICE,
        DESTRUCTION_SHOCK_ADEPT,
        DESTRUCTION_SHOCK_EXPERT,
        DESTRUCTION_SHOCK_MASTER,
        DESTRUCTION_GENERIC_NOVICE,
        DESTRUCTION_GENERIC_APPRENTICE,
        DESTRUCTION_GENERIC_ADEPT,
        DESTRUCTION_GENERIC_EXPERT,
        DESTRUCTION_GENERIC_MASTER,
        ALTERATION_NOVICE,
        ALTERATION_APPRENTICE,
        ALTERATION_ADEPT,
        ALTERATION_EXPERT,
        ALTERATION_MASTER,
        RESTORATION_FRIENDLY_NOVICE,
        RESTORATION_FRIENDLY_APPRENTICE,
        RESTORATION_FRIENDLY_ADEPT,
        RESTORATION_FRIENDLY_EXPERT,
        RESTORATION_FRIENDLY_MASTER,
        RESTORATION_HOSTILE_NOVICE,
        RESTORATION_HOSTILE_APPRENTICE,
        RESTORATION_HOSTILE_ADEPT,
        RESTORATION_HOSTILE_EXPERT,
        RESTORATION_HOSTILE_MASTER,
        ILLUSION_FRIENDLY_NOVICE,
        ILLUSION_FRIENDLY_APPRENTICE,
        ILLUSION_FRIENDLY_ADEPT,
        ILLUSION_FRIENDLY_EXPERT,
        ILLUSION_FRIENDLY_MASTER,
        ILLUSION_HOSTILE_NOVICE,
        ILLUSION_HOSTILE_APPRENTICE,
        ILLUSION_HOSTILE_ADEPT,
        ILLUSION_HOSTILE_EXPERT,
        ILLUSION_HOSTILE_MASTER,
        CONJURATION_BOUND_WEAPON_NOVICE,
        CONJURATION_BOUND_WEAPON_APPRENTICE,
        CONJURATION_BOUND_WEAPON_ADEPT,
        CONJURATION_BOUND_WEAPON_EXPERT,
        CONJURATION_BOUND_WEAPON_MASTER,
        CONJURATION_SUMMON_NOVICE,
        CONJURATION_SUMMON_APPRENTICE,
        CONJURATION_SUMMON_ADEPT,
        CONJURATION_SUMMON_EXPERT,
        CONJURATION_SUMMON_MASTER,
        SHOUT_GENERIC,
    };

    struct Gametime_cooldown_value {
        float readytime; //Gametime when ready again
        float duration; //total cooldown duration

        Gametime_cooldown_value(float readytime, float duration);
        float get_progress(float current_game_time);
    };

    struct Spell_cast_data {
        float gcd;
        float cooldown;
        float casttime;
        uint16_t animation;
        uint16_t casteffectid;
        
        Spell_cast_data();
        bool is_empty();
    };


    enum class custom_transform_spell_type: uint8_t {
        regular = 0U, //regular inventory
        fav_menu, //use fave menu to bind, but still cast all trough bar
        fav_menu_switch // use fav menu, and switch spells instead of casting
    };

    struct Transformation_data {
        uint32_t bar_id;
        custom_transform_spell_type casting_type;
    };

    extern RE::BGSListForm* spell_keybinds_list;
    extern RE::TESGlobal* global_animation_type;
    extern RE::TESGlobal* global_vampire_lord_equip_mode;

    extern std::unordered_map<RE::FormID, Spell_cast_data> spell_cast_info;
    extern std::vector<std::pair<RE::BGSArtObject*, RE::BGSArtObject*>> spell_casteffect_art;

    void load_from_SKSE_save(SKSE::SerializationInterface* a_intfc);
    void save_to_SKSE_save(SKSE::SerializationInterface* a_intfc);

    //Called when game data is available
    void onDataLoad();

    //read the currently bound key for a spell from global var
    int get_spell_keybind(int slot_index);

    std::string get_keybind_text(int slot_index, key_modifier mod);

    inline RE::TESForm* get_form_from_file(const uint32_t formID, const std::string& pluginFile)
    {
        static const auto data_handler = RE::TESDataHandler::GetSingleton();
        if (data_handler != nullptr) {
            return data_handler->LookupForm(formID, pluginFile);
        } else {
            logger::error("data_handler is null");
            return nullptr;
        }
    }

    void set_spell_cast_data(RE::FormID spell, Spell_cast_data&& data);

    void reload_data();

    void set_next_allowed_cast(float seconds);
    bool is_allowed_to_cast();
    float get_current_gcd_progress();
    float get_current_gcd_duration();
    void update_gcd_timer(float delta);

    void add_gametime_cooldown(RE::FormID skill, float hours);

    void add_gametime_cooldown_with_timescale(RE::FormID skill, float days);

    void purge_expired_gametime_cooldowns();

    std::tuple<float, float> get_gametime_cooldown(float curr_game_time, RE::FormID skill);

    std::string resolve_spellname(RE::FormID formID);

    EquippedType getPlayerEquipmentType();
    bool isVampireLord();
    bool isWerewolf();
    uint32_t isCustomTransform();
    custom_transform_spell_type getCustomTransformCasttype();

    bool hasFavMenuSlotBinding();

    std::pair<bool, float> shouldShowHUDBar();

    DefaultIconType get_fallback_icon_type(RE::TESForm* form);

    void add_casteffect(const std::string& key, RE::BGSArtObject* left_art, RE::BGSArtObject* right_art);
    /*
    * Should only be called during spell data loading, otherwise resolution map will be null
    */
    size_t get_cast_effect_id(const std::string& key);

    void update_spell_casting_art_and_time(size_t art_index, uint32_t casttime);

    /**
     * Get spell at index of current ingame bar
     */
    RE::FormID get_current_spell_in_slot(size_t index);

    /*
    * Is the passed spell the "unbind slot" spell?
    */
    bool is_clear_spell(RE::FormID spell);

    void add_custom_tranformation(uint32_t bar, std::string name, RE::FormID race_id,
                                  custom_transform_spell_type cast_type);

}