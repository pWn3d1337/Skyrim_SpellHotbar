#include "game_data.h"
#include "../logger/logger.h"
#include "../bar/hotbars.h"
#include "../rendering/render_manager.h"
#include "spell_data_csv_loader.h"
#include "spell_casteffect_csv_loader.h"
#include "custom_transform_csv_loader.h"

namespace SpellHotbar::GameData {

    constexpr std::string_view spell_data_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\spelldata\\";
    constexpr std::string_view spell_casteffects_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\effectdata\\";
    constexpr std::string_view custom_transformations_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\transformdata\\";

    RE::BGSListForm* spell_keybinds_list = nullptr;

    RE::TESRace* vampire_lord_race = nullptr;
    RE::TESRace* werewolf_beast_race = nullptr;

    RE::TESGlobal* global_animation_type = nullptr;
    RE::TESGlobal* global_vampire_lord_equip_mode = nullptr;

    RE::SpellItem* spellhotbar_castfx_spell = nullptr;
    RE::SpellItem* spellhotbar_unbind_slot = nullptr;

    float gcd_timer{0.0};
    float gcd_total{0.0};

    std::unordered_map<RE::FormID, Spell_cast_data> spell_cast_info;
    std::vector<std::pair<RE::BGSArtObject*, RE::BGSArtObject*>> spell_casteffect_art;
    std::unordered_map<RE::FormID, Gametime_cooldown_value> gametime_cooldowns;

    std::unique_ptr<std::unordered_map<std::string, size_t>> spell_effects_key_indices{nullptr};

    std::unordered_map<RE::FormID, Transformation_data> custom_transformation_data;

    const std::unordered_map<int, std::string> key_names = {
        {1, "Esc"},    {2, "1"},       {3, "2"},       {4, "3"},      {5, "4"},        {6, "5"},      {7, "6"},
        {8, "7"},      {9, "8"},       {10, "9"},      {11, "0"},     {12, "-"},       {13, "="},     {14, "Bsp"},
        {15, "Tab"},   {16, "Q"},      {17, "W"},      {18, "E"},     {19, "R"},       {20, "T"},     {21, "Y"},
        {22, "U"},     {23, "I"},      {24, "O"},      {25, "P"},     {26, "("},       {27, ")"},     {28, "Enter"},
        {29, "LCtrl"}, {30, "A"},      {31, "S"},      {32, "D"},     {33, "F"},       {34, "G"},     {35, "H"},
        {36, "J"},     {37, "K"},      {38, "L"},      {39, ","},     {40, "'"},       {41, "~"},     {42, "LShift"},
        {43, "\\"},    {44, "Z"},      {45, "X"},      {46, "C"},     {47, "V"},       {48, "B"},     {49, "N"},
        {50, "M"},     {51, ","},      {52, "."},      {53, "/"},     {54, "RShift"},  {55, "NP*"},   {56, "LAlt"},
        {57, "Space"}, {58, "Caps"},   {59, "F1"},     {60, "F2"},    {61, "F3"},      {62, "F4"},    {63, "F5"},
        {64, "F6"},    {65, "F7"},     {66, "F8"},     {67, "F9"},    {68, "F10"},     {69, "Num"},   {70, "Scroll"},
        {71, "NP7"},   {72, "NP8"},    {73, "NP9"},    {74, "NP-"},   {75, "NP4"},     {76, "NP5"},   {77, "NP6"},
        {78, "NP+"},   {79, "NP1"},    {80, "NP2"},    {81, "NP3"},   {82, "NP0"},     {83, "NP."},   {87, "F11"},
        {88, "F12"},   {156, "NPEnt"}, {157, "RCtrl"}, {181, "NP/"},  {183, "PtrScr"}, {184, "RAlt"}, {197, "Pause"},
        {199, "Home"}, {200, "Up"},    {201, "PgUp"},  {203, "Left"}, {205, "Right"},  {207, "End"},  {208, "Down"},
        {209, "PgDn"}, {210, "Ins"},   {211, "Del"},   {256, "LMB"},  {257, "RMB"},    {258, "MMB"},  {259, "M3"},
        {260, "M4"},   {261, "M5"},    {262, "M6"},    {263, "M7"},   {264, "MWUp"},   {265, "MWDn"},
    };

    void onDataLoad() 
    { 
        auto keybinds_formlist = get_form_from_file(0x80D, "SpellHotbar.esp");
        if (keybinds_formlist == nullptr)
        {
            logger::error("FormList 'SpellHotbar_Keybinds' from 'SpellHotbar.esp' is null!");
        }
        else
        {
            spell_keybinds_list = keybinds_formlist->As<RE::BGSListForm>();
            if (!spell_keybinds_list) {
                logger::error("Spell Keybinds list is null!");
            }
        }

        auto vl = get_form_from_file(0x00283A, "Dawnguard.esm");
        if (vl) {
            vampire_lord_race = vl->As<RE::TESRace>();
        } else {
            logger::error("Could not get Vampire Lord Race from Dawnguard.esm!");
        }

        auto ww = get_form_from_file(0xCDD84, "Skyrim.esm");
        if (ww) {
            werewolf_beast_race = ww->As<RE::TESRace>();
        } else {
            logger::error("Could not get Werewolf Beast Race from Skyrim.esm!");
        }

        auto spell_anim = get_form_from_file(0x815, "SpellHotbar.esp");
        if (spell_anim) {
            global_animation_type = spell_anim->As<RE::TESGlobal>();
        } else {
            logger::error("Could not get 'SpellHotbar_SpellAnimationType' from SpellHotbar.esp");
        }

        auto vampire_lord_equip_mode = get_form_from_file(0x820, "SpellHotbar.esp");
        if (vampire_lord_equip_mode) {
            global_vampire_lord_equip_mode = vampire_lord_equip_mode->As<RE::TESGlobal>();
        } else {
            logger::error("Could not get 'SpellHotbar_VampireLordUseEquipMode' from SpellHotbar.esp");
        }

        auto hand_effect_spell_form = SpellHotbar::GameData::get_form_from_file(0x816, "SpellHotbar.esp");
        if (hand_effect_spell_form && hand_effect_spell_form->GetFormType() == RE::FormType::Spell) {
            spellhotbar_castfx_spell = hand_effect_spell_form->As<RE::SpellItem>();
        } else {
            logger::error("Could not get 'SpellHotbar_CastFX_Spell' from SpellHotbar.esp");
        }

        auto unbind_spell_form = SpellHotbar::GameData::get_form_from_file(0x810, "SpellHotbar.esp");
        if (unbind_spell_form && unbind_spell_form->GetFormType() == RE::FormType::Spell) {
            spellhotbar_unbind_slot = unbind_spell_form->As<RE::SpellItem>();
        } else {
            logger::error("Could not get 'SpellHotbar_unbind' from SpellHotbar.esp");
        }

        //need to wait for game data beeing available
        RenderManager::load_gamedata_dependant_resources();

        spell_effects_key_indices = std::make_unique<std::unordered_map<std::string, size_t>>();
        SpellCastEffectCSVLoader::load_spell_casteffects(std::filesystem::path(spell_casteffects_root));
        SpellDataCSVLoader::load_spell_data(std::filesystem::path(spell_data_root));

        spell_effects_key_indices = nullptr; //no longer need this

        //load custom transformations
        CustomTransformCSVLoader::load_transformations(std::filesystem::path(custom_transformations_root));
    }

    void reload_data()
    { 
        logger::info("Reloading Spell Data...");
        spell_cast_info.clear();
        spell_casteffect_art.clear();

        spell_effects_key_indices = std::make_unique<std::unordered_map<std::string, size_t>>();
        SpellCastEffectCSVLoader::load_spell_casteffects(std::filesystem::path(spell_casteffects_root));
        SpellDataCSVLoader::load_spell_data(std::filesystem::path(spell_data_root));
        spell_effects_key_indices = nullptr;  // no longer need this
    }

    int get_spell_keybind(int slot_index)
    {
        if (!spell_keybinds_list) return 0;
        auto var = spell_keybinds_list->forms[slot_index]->As<RE::TESGlobal>();
        if (var) {
            return static_cast<int>(var->value);
        } else {
            logger::info("var is null");
            return 0;
        }
    }

    std::string get_keybind_text(int slot_index, key_modifier mod)
    {
        int keybind = get_spell_keybind(slot_index);

        std::string key_text;
        if (key_names.contains(keybind)) {
            key_text = key_names.at(keybind);

            switch (mod) {
                case SpellHotbar::key_modifier::ctrl:
                    key_text = "C-" + key_text;
                    break;
                case SpellHotbar::key_modifier::shift:
                    key_text = "S-" + key_text;
                    break;
                case SpellHotbar::key_modifier::alt:
                    key_text = "A-" + key_text;
                    break;
                default:
                    break;
            }

        } else {
            key_text = "??";
        }
        return key_text;
    }

    EquippedType getPlayerEquipmentType()
    { 
        auto pc = RE::PlayerCharacter::GetSingleton();
        RE::TESForm* right_hand = pc->GetEquippedObject(false);
        RE::TESForm* left_hand = pc->GetEquippedObject(true);

        if (right_hand == nullptr)
        {
            return EquippedType::FIST;
        }
        else if (right_hand->GetFormType() == RE::FormType::Weapon)
        {
            RE::TESObjectWEAP* weapon_right = right_hand->As<RE::TESObjectWEAP>();
            RE::WEAPON_TYPE type = weapon_right->GetWeaponType();
            switch (type)
            { 
            case RE::WEAPON_TYPE::kHandToHandMelee:
                    return EquippedType::FIST;
            case RE::WEAPON_TYPE::kTwoHandAxe:
            case RE::WEAPON_TYPE::kTwoHandSword:
                    return EquippedType::TWOHAND;
            case RE::WEAPON_TYPE::kStaff:
                    return EquippedType::SPELL;
            case RE::WEAPON_TYPE::kBow:
            case RE::WEAPON_TYPE::kCrossbow:
                    return EquippedType::RANGED;
            case RE::WEAPON_TYPE::kOneHandAxe:
            case RE::WEAPON_TYPE::kOneHandDagger:
            case RE::WEAPON_TYPE::kOneHandMace:
            case RE::WEAPON_TYPE::kOneHandSword:
                //need to check left hand
                    if (left_hand == nullptr) return EquippedType::ONEHAND_EMPTY;
                    if (left_hand->GetFormType() == RE::FormType::Weapon) {
                        RE::TESObjectWEAP* weapon_left = left_hand->As<RE::TESObjectWEAP>();
                        if (weapon_left->IsMelee()) {
                            return EquippedType::DUAL_WIELD;
                        }
                        else
                        {
                            //If not empty, weapon record and no melee weapon -> treat as 1h + spell
                            return EquippedType::ONEHAND_SPELL;
                        } 
                    } else if (left_hand->GetFormType() == RE::FormType::Armor){
                        return EquippedType::ONEHAND_SHIELD;
                    } else if (left_hand->GetFormType() == RE::FormType::Spell) {
                        return EquippedType::ONEHAND_SPELL;
                    }
            }
        }
        else if (right_hand->GetFormType() == RE::FormType::Spell)
        {
            return EquippedType::SPELL;
        }
        //std::string r = right_hand ? right_hand->GetName() : "empty";
        //std::string l = left_hand ? left_hand->GetName() : "empty";



        //bool weapon_drawn = pc->AsActorState()->IsWeaponDrawn();
        //bool sneaking = pc->AsActorState()->IsSneaking();
        //logger::trace("Weapon Drawn: {}", weapon_drawn);
        //logger::trace("RightHand: {}, LeftHand: {}", r, l);
        return EquippedType::FIST;
    }

    bool isVampireLord()
    { 
        auto * pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return false;
        return (vampire_lord_race != nullptr) && (pc->GetRace() == vampire_lord_race);
    }
    bool isWerewolf() {
        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return false;
        return (werewolf_beast_race != nullptr) && (pc->GetRace() == werewolf_beast_race);
    }

    uint32_t isCustomTransform() {
        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return 0U;
        RE::FormID currentRace = pc->GetRace()->GetFormID();

        if (custom_transformation_data.contains(currentRace)) {
            auto &dat = custom_transformation_data.at(currentRace);
            return dat.bar_id;
        }
        return 0U;
    }

    custom_transform_spell_type SpellHotbar::GameData::getCustomTransformCasttype() {
        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return custom_transform_spell_type::regular;
        RE::FormID currentRace = pc->GetRace()->GetFormID();

        if (custom_transformation_data.contains(currentRace)) {
            auto& dat = custom_transformation_data.at(currentRace);
            return dat.casting_type;
        }
        return custom_transform_spell_type::regular;
    }

    bool hasFavMenuSlotBinding() {
        bool ret = isVampireLord() || isWerewolf();
        if (!ret && isCustomTransform()) {
            auto casttype = GameData::getCustomTransformCasttype();
            if (casttype == GameData::custom_transform_spell_type::fav_menu ||
                casttype == GameData::custom_transform_spell_type::fav_menu_switch) {
                ret = true;
            }
        }
        return ret;
    }

    std::string resolve_spellname(RE::FormID formID) {
        if (formID == 0) return "EMPTY";
        if (GameData::is_clear_spell(formID)) return "BLOCKED";

        auto form = RE::TESForm::LookupByID(formID);
        if (form == nullptr) {
            return "<INVALID>";
        } else {
            return form->GetName();
        }
    }

    std::pair<bool, float> shouldShowHUDBar() {
        constexpr float fast_fade = 0.1f;
        constexpr float slow_fade = 0.5f;
        //credits to LamasTinyHUD
        auto* ui = RE::UI::GetSingleton(); 
        if (!ui || ui->GameIsPaused() || !ui->IsCursorHiddenWhenTopmost() || !ui->IsShowingMenus() || !ui->GetMenu<RE::HUDMenu>() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME))
        {
            return std::make_pair(false, fast_fade);
        }

        //crashes on 1170
        /*if (const auto* control_map = RE::ControlMap::GetSingleton();
            !control_map || !control_map->IsMovementControlsEnabled() ||
            (control_map->contextPriorityStack.size() > 0 && control_map->contextPriorityStack.back() != RE::UserEvents::INPUT_CONTEXT_ID::kGameplay)) {
            return std::make_pair(false, fast_fade);
        }*/
        //not sure what exactly contextPriorityStack does anyway
        if (const auto* control_map = RE::ControlMap::GetSingleton();
            !control_map || !control_map->IsMovementControlsEnabled())
        { 
            return std::make_pair(false, fast_fade);
        }

        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (pc) {
            if (GameData::isVampireLord()) {
                switch (Bars::bar_show_setting_vampire_lord) {
                    case Bars::bar_show_mode::always:
                        return std::make_pair(true, slow_fade);

                    case Bars::bar_show_mode::combat:
                        return std::make_pair(pc->IsInCombat(), slow_fade);

                    default:
                        return std::make_pair(false, slow_fade);
                }
            } else if (GameData::isWerewolf()) {
                switch (Bars::bar_show_setting_werewolf) {
                    case Bars::bar_show_mode::always:
                        return std::make_pair(true, slow_fade);

                    case Bars::bar_show_mode::combat:
                        return std::make_pair(pc->IsInCombat(), slow_fade);

                    default:
                        return std::make_pair(false, slow_fade);
                }
            } else {
                if (Bars::bar_show_setting == Bars::bar_show_mode::never) return std::make_pair(false, slow_fade);
                if (Bars::bar_show_setting == Bars::bar_show_mode::always) return std::make_pair(true, slow_fade);

                bool in_combat = pc->IsInCombat();
                bool weapon_drawn = pc->AsActorState()->IsWeaponDrawn();

                switch (Bars::bar_show_setting) {
                    case Bars::bar_show_mode::combat:
                        return std::make_pair(in_combat, slow_fade);

                    case Bars::bar_show_mode::drawn_weapon:
                        return std::make_pair(weapon_drawn, slow_fade);

                    case Bars::bar_show_mode::combat_and_drawn:
                        return std::make_pair(in_combat && weapon_drawn, slow_fade);

                    case Bars::bar_show_mode::combat_or_drawn:
                        return std::make_pair(in_combat || weapon_drawn, slow_fade);
                }
            }
        }
        return std::make_pair(false, fast_fade);
    }

    inline int get_spell_rank(int32_t minlevel) {
        int ret{0};
        if (minlevel >= 100) {
            ret = 4;
        } else if (minlevel >= 75) {
            ret = 3;
        } else if (minlevel >= 50) {
            ret = 2;
        } else if (minlevel >= 25) {
            ret = 1;
        }
        return ret;
    }

    DefaultIconType get_fallback_icon_type(RE::TESForm* form)
    { 
        DefaultIconType ret {DefaultIconType::UNKNOWN};

        if (form->GetFormType() == RE::FormType::Spell) {
            RE::SpellItem* spell = form->As<RE::SpellItem>();
            if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
            
                ret = DefaultIconType::LESSER_POWER;
            } else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower) {
                
                ret = DefaultIconType::GREATER_POWER;
            } else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                if (spell->effects.size() > 0U) {

                    for (RE::BSTArrayBase::size_type i = 0U; i < spell->effects.size() && ret == DefaultIconType::UNKNOWN; i++) {
                        //find first spell effect that has a magic school
                        RE::Effect* effect = spell->effects[i];
                        if (effect->baseEffect) {
                            RE::ActorValue school = effect->baseEffect->GetMagickSkill();
                            int rank = get_spell_rank(effect->baseEffect->GetMinimumSkillLevel());
                            RE::ActorValue dmg_type = effect->baseEffect->data.resistVariable;

                            switch (school) {
                                case RE::ActorValue::kDestruction:
                                    if (dmg_type == RE::ActorValue::kResistFire) {
                                        ret = DefaultIconType::DESTRUCTION_FIRE_NOVICE;
                                    } else if (dmg_type == RE::ActorValue::kResistFrost) {
                                        ret = DefaultIconType::DESTRUCTION_FROST_NOVICE;
                                    } else if (dmg_type == RE::ActorValue::kResistShock) {
                                        ret = DefaultIconType::DESTRUCTION_SHOCK_NOVICE;
                                    } else {
                                        ret = DefaultIconType::DESTRUCTION_GENERIC_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;

                                case RE::ActorValue::kAlteration:
                                    ret = DefaultIconType(static_cast<uint32_t>(DefaultIconType::ALTERATION_NOVICE) +
                                                          rank);
                                    break;

                                case RE::ActorValue::kIllusion:
                                    if (effect->baseEffect->data.flags.all(
                                            RE::EffectSetting::EffectSettingData::Flag::kHostile)) {
                                        ret = DefaultIconType::ILLUSION_HOSTILE_NOVICE;
                                    } else {
                                        ret = DefaultIconType::ILLUSION_FRIENDLY_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;

                                case RE::ActorValue::kRestoration:
                                    if (effect->baseEffect->data.flags.all(
                                            RE::EffectSetting::EffectSettingData::Flag::kHostile)) {
                                        ret = DefaultIconType::RESTORATION_HOSTILE_NOVICE;
                                    } else {
                                        ret = DefaultIconType::RESTORATION_FRIENDLY_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;

                                case RE::ActorValue::kConjuration:
                                    auto archetype = effect->baseEffect->data.archetype;
                                    if (archetype == RE::EffectSetting::Archetype::kBoundWeapon) {
                                        ret = DefaultIconType::CONJURATION_BOUND_WEAPON_NOVICE;
                                    } else if (archetype == RE::EffectSetting::Archetype::kSummonCreature ||
                                               archetype == RE::EffectSetting::Archetype::kReanimate) {
                                        ret = DefaultIconType::CONJURATION_SUMMON_NOVICE;
                                    } else {
                                        ret = DefaultIconType::CONJURATION_SUMMON_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;
                            }
                        }
                    }
                }
            }
        }
        else if (form->GetFormType() == RE::FormType::Shout) {
            //RE::TESShout* shout = form->As<RE::TESShout>();
            ret = DefaultIconType::SHOUT_GENERIC;
        }
        return ret;
    }

    void set_spell_cast_data(RE::FormID spell, Spell_cast_data&& data) {
        auto it = spell_cast_info.find(spell);
        if (it != spell_cast_info.end()) {
            it->second = std::move(data);
        }
        else {
            spell_cast_info.emplace(spell, std::move(data));
        }
    }

    void set_next_allowed_cast(float seconds)
    { 
        gcd_timer = seconds;
        gcd_total = seconds;
    }

    void save_to_SKSE_save(SKSE::SerializationInterface* a_intfc)
    {
        //purge expired cooldowns before saving
        purge_expired_gametime_cooldowns();

        //write cooldowns size, also write 0 so its known there is nothing else to load
        uint32_t size = static_cast<uint32_t>(gametime_cooldowns.size()); //32bit will be enough...
        a_intfc->WriteRecordData(&size, sizeof(uint32_t));

        for (const auto& [id, cd] : gametime_cooldowns) {
            a_intfc->WriteRecordData(&id, sizeof(RE::FormID));     
            a_intfc->WriteRecordData(&cd.readytime, sizeof(float));    
            a_intfc->WriteRecordData(&cd.duration, sizeof(float));  
        }

    }

    void load_from_SKSE_save(SKSE::SerializationInterface* a_intfc) {
        uint32_t size;
        if (!a_intfc->ReadRecordData(&size, sizeof(uint32_t)))
        {
            logger::error("Error reading Cooldown map size!");
        } else {
            gametime_cooldowns.clear();
            auto cal = RE::Calendar::GetSingleton();

            for (uint32_t i = 0Ui32; i < size; i++) {
                RE::FormID id{0};
                float ready{0.0f};
                float dur{0.0f};

                if (!a_intfc->ReadRecordData(&id, sizeof(RE::FormID))) {
                    logger::error("Failed to read cooldown entry!");
                    break;
                }
                if (!a_intfc->ReadRecordData(&ready, sizeof(float))) {
                    logger::error("Failed to read cooldown entry!");
                    break;
                }
                if (!a_intfc->ReadRecordData(&dur, sizeof(float))) {
                    logger::error("Failed to read cooldown entry!");
                    break;
                }
                if (cal) {
                    if (cal->GetCurrentGameTime() < ready) {
                        gametime_cooldowns.emplace(id, Gametime_cooldown_value(ready, dur));
                    } else {
                        logger::warn("Expired cooldown was saved, skipping!");
                    }
                } else {
                    logger::error("Game Calendar is null");
                }
            }
        }
    }

    bool is_allowed_to_cast()
    {
        if (gcd_total > 0.0f) {
            return gcd_timer <= 0.0f;
        }
        return true;
    }

    float get_current_gcd_progress() {
        if (gcd_total > 0.0f) {
            return 1.0f - (gcd_timer / gcd_total);
        }
        return 0.0f;
    }

    void update_gcd_timer(float delta) 
    {
        if (gcd_timer > 0.0f) {
            gcd_timer -= delta;
            if (gcd_timer <= 0.0f) {
                gcd_timer = 0.0f;
                gcd_total = 0.0f;
                if (global_animation_type) {
                    //Reset animation variable if gcd expires
                    global_animation_type->value = 0;
                }
            }
        }
    }
    void add_gametime_cooldown(RE::FormID skill, float hours)
    { 
        auto cal = RE::Calendar::GetSingleton();
        if (cal) {
            float gt_value = hours / 24.0f;
            gametime_cooldowns.emplace(skill, Gametime_cooldown_value(cal->GetCurrentGameTime() + gt_value, gt_value));
        }
    }

    void add_gametime_cooldown_with_timescale(RE::FormID skill, float days) {
        auto cal = RE::Calendar::GetSingleton();
        if (cal) {
            float gt_value = days * cal->GetTimescale();
            gametime_cooldowns.emplace(skill, Gametime_cooldown_value(cal->GetCurrentGameTime() + gt_value, gt_value));
        }
    }

    void purge_expired_gametime_cooldowns() {
        auto cal = RE::Calendar::GetSingleton();
        float gt = cal->GetCurrentGameTime();
        if (cal) {
            std::erase_if(gametime_cooldowns, [gt](const auto& elem)
                {
                    auto const& [key, value] = elem;
                    return value.readytime < gt;
                });
        } else {
            logger::warn("Could not purge cooldowns, calendar was null");
        }
    }

    std::tuple<float, float> get_gametime_cooldown(float curr_game_time, RE::FormID skill)
    {
        if (gametime_cooldowns.contains(skill)) {
            auto iter = gametime_cooldowns.find(skill);
            auto & cd_values = iter->second;

            if (cd_values.readytime <= curr_game_time) {
                //Cooldown is finished
                //logger::info("Removing CD for {} from map, finished!", skill);
                gametime_cooldowns.erase(iter);
            } else {
                float prog = cd_values.get_progress(curr_game_time);
                return std::make_tuple(prog, cd_values.duration);
            }
        }
        return std::make_tuple(0.0f, 0.0f);
    }

    float get_current_gcd_duration()
    { 
        return gcd_total;
    }

    Gametime_cooldown_value::Gametime_cooldown_value(float time, float dur) : readytime(time), duration(dur){};

    float Gametime_cooldown_value::get_progress(float curr_time) {
        if (readytime > curr_time) {
            return 1.0f - ((readytime - curr_time) / duration);
        }
        return 0.0f;
    }

    Spell_cast_data::Spell_cast_data() : gcd(-1.0f), cooldown(-1.0f), casttime(-1.0f), animation(0), casteffectid{0} {}
    bool Spell_cast_data::is_empty()
    { 
        return animation == 0U && gcd < 0.0f && cooldown < 0.0f && casttime < 0.0f && casteffectid == 0U;
    }

    RE::NiPoint3 vectorMatrixMult(const RE::NiPoint3& a_vector, const RE::NiMatrix3& a_matrix) {
        return RE::NiPoint3(
            a_matrix.entry[0][0] * a_vector.x + a_matrix.entry[0][1] * a_vector.y + a_matrix.entry[0][2] * a_vector.z,
            a_matrix.entry[1][0] * a_vector.x + a_matrix.entry[1][1] * a_vector.y + a_matrix.entry[1][2] * a_vector.z,
            a_matrix.entry[2][0] * a_vector.x + a_matrix.entry[2][1] * a_vector.y + a_matrix.entry[2][2] * a_vector.z);
    }

    void add_casteffect(const std::string& key, RE::BGSArtObject* left_art, RE::BGSArtObject* right_art) 
    {
        spell_casteffect_art.emplace_back(std::make_pair(left_art, right_art));
        if (spell_effects_key_indices) {
            spell_effects_key_indices.get()->insert(std::make_pair(key, spell_casteffect_art.size()-1));
        } else {
            logger::error("Spell effect key index map is null during load!");
        }
    }

    size_t get_cast_effect_id(const std::string& key)
    {
        size_t ret{0};
        if (spell_effects_key_indices) {
            if (spell_effects_key_indices.get()->contains(key)) {
                ret = spell_effects_key_indices.get()->at(key);
            } else {
                logger::warn("Unknown Casteffect: {}", key);
            }
        } else {
            logger::error("Spell effect key index map is called at incorrect time!");
        }
        return ret;
    }

    void update_spell_casting_art_and_time(size_t art_index, uint32_t casttime) { 
        if (spellhotbar_castfx_spell)
        {
            spellhotbar_castfx_spell->effects[0]->effectItem.duration = casttime;
            spellhotbar_castfx_spell->effects[1]->effectItem.duration = casttime;

            if (art_index >= spell_casteffect_art.size()) {
                art_index = 0U;
            }
            const auto& effect = spell_casteffect_art.at(art_index);
            spellhotbar_castfx_spell->effects[0]->baseEffect->data.hitEffectArt = effect.first;
            spellhotbar_castfx_spell->effects[1]->baseEffect->data.hitEffectArt = effect.second;
        }
    }

     RE::FormID get_current_spell_in_slot(size_t index) {
        RE::FormID ret{0};
        if (index < max_bar_size) {
            uint32_t bar_id = Bars::getCurrentHotbar_ingame();
            if (Bars::hotbars.contains(bar_id)) {
                auto& bar = Bars::hotbars.at(bar_id);
                auto [id, type, inherited] = bar.get_skill_in_bar_with_inheritance(static_cast<int>(index), Bars::get_current_modifier(), false);
                ret = id;
            }
        }
        if (GameData::is_clear_spell(ret)) {
            ret = 0U;
        }
        return ret;
    }

     bool is_clear_spell(RE::FormID spell)
     { 
         if (spellhotbar_unbind_slot)
         {
             return spell == spellhotbar_unbind_slot->formID;
         }
         return false;
     }

     void add_custom_tranformation(uint32_t bar, std::string name, RE::FormID race_id, custom_transform_spell_type cast_type) 
     {
         custom_transformation_data.insert(std::make_pair(race_id, Transformation_data(bar, cast_type)));

         if (!Bars::hotbars.contains(bar)) {
             if (!Bars::bar_names.contains(bar)) {
                Bars::bar_names[bar] = name;
             }
             Bars::add_special_bar(bar);
         }
     }
}
