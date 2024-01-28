#include "papyrus_functions.h"
#include "../storage/storage.h"
#include "../logger/logger.h"
#include "../bar/hotbars.h"
#include "../bar/hotbar.h"
#include "../game_data/game_data.h"
#include "../rendering/render_manager.h"

RE::TESForm* get_current_selected_spell_in_menu(RE::StaticFunctionTag*) {

    RE::UI* ui = RE::UI::GetSingleton();
    if (!ui) return nullptr;

    if (!SpellHotbar::GameData::hasFavMenuSlotBinding()) {
        // totally stolen from Wheeler
        auto* magMenu = static_cast<RE::MagicMenu*>(ui->GetMenu(RE::MagicMenu::MENU_NAME).get());
        if (!magMenu) return nullptr;

        RE::GFxValue selection;
        magMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId");
        if (selection.GetType() == RE::GFxValue::ValueType::kNumber) {
            RE::FormID formID = static_cast<std::uint32_t>(selection.GetNumber());
            return RE::TESForm::LookupByID(formID);
        }
    } else {
        auto* favMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(RE::FavoritesMenu::MENU_NAME).get());
        if (!favMenu) return nullptr;

        auto & root = favMenu->GetRuntimeData().root;

        if (root.GetType() == RE::GFxValue::ValueType::kDisplayObject && root.HasMember("itemList")) {
            RE::GFxValue itemList;
            root.GetMember("itemList", &itemList);

            if (itemList.GetType() == RE::GFxValue::ValueType::kDisplayObject && itemList.HasMember("selectedEntry")) {
                RE::GFxValue selectedEntry;
                itemList.GetMember("selectedEntry", &selectedEntry);

                if (selectedEntry.GetType() == RE::GFxValue::ValueType::kObject && selectedEntry.HasMember("formId")) {
                    RE::GFxValue formId;
                    selectedEntry.GetMember("formId", &formId);

                    if (formId.GetType() == RE::GFxValue::ValueType::kNumber) {
                        RE::FormID formID = static_cast<std::uint32_t>(formId.GetNumber());
                        return RE::TESForm::LookupByID(formID);
                    }
                }
            }
        }

        // For some reason the path to the variable does not work that way
        //RE::GFxValue selection;
        //favMenu->uiMovie->GetVariable(&selection, "_root.itemList.selectedEntry.formId");  //.selectedEntry.formId");
        //favMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.itemList.selectedEntry.formId");  //.selectedEntry.formId");
        //favMenu->uiMovie->GetVariable(&selection, "itemList.selectedEntry.formId");  //.selectedEntry.formId");
        //
        //if (selection.GetType() == RE::GFxValue::ValueType::kNumber) {
        //    RE::FormID formID = static_cast<std::uint32_t>(selection.GetNumber());
        //    return RE::TESForm::LookupByID(formID);
        //} else {
        //    logger::info(":-(");
        //}
    }
    return nullptr;
}

RE::TESForm* get_slotted_spell(RE::StaticFunctionTag*, int32_t index) {
    //logger::trace("Retrieving slotted spell {}", index);
    RE::FormID formID = SpellHotbar::GameData::get_current_spell_in_slot(index);
    if (formID > 1) {
        return RE::TESForm::LookupByID(formID);
    }
    return nullptr;
}

bool is_ctrl_enabled(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::enable_ctrl_bars;
}

bool toggle_ctrl_enabled(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::enable_ctrl_bars = !SpellHotbar::Bars::enable_ctrl_bars;
}

bool is_shift_enabled(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::enable_shift_bars;
}

bool toggle_shift_enabled(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::enable_shift_bars = !SpellHotbar::Bars::enable_shift_bars;
}

bool is_alt_enabled(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::enable_alt_bars;
}

bool toggle_alt_enabled(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::enable_alt_bars = !SpellHotbar::Bars::enable_alt_bars;
}

int get_number_of_slots(RE::StaticFunctionTag*)
{
    return static_cast<int>(SpellHotbar::Bars::barsize);
}

int set_number_of_slots(RE::StaticFunctionTag*, int num)
{
    SpellHotbar::Bars::barsize = static_cast<uint8_t>(std::clamp(num, 1, static_cast<int>(SpellHotbar::max_bar_size)));
    return static_cast<int>(SpellHotbar::Bars::barsize);
}

float get_slot_scale(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::slot_scale;
}

float set_slot_scale(RE::StaticFunctionTag*, float scale)
{
    return SpellHotbar::Bars::slot_scale = scale;
}

float get_offset_x(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::offset_x;
}
float set_offset_x(RE::StaticFunctionTag*, float value)
{
    return SpellHotbar::Bars::offset_x = value;
}

float get_offset_y(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::offset_y;
}
float set_offset_y(RE::StaticFunctionTag*, float value)
{
    return SpellHotbar::Bars::offset_y = value;
}

bool slot_spell(RE::StaticFunctionTag*, RE::TESForm* form, int32_t index, int type) {
    if (form != nullptr)
    {
        if (type >= 0 && type <= 3) {
            SpellHotbar::Storage::menu_slot_type slot_type = static_cast<SpellHotbar::Storage::menu_slot_type>(type);
            if (SpellHotbar::GameData::isCustomTransform()) {
                auto casttype = SpellHotbar::GameData::getCustomTransformCasttype();     
                if (casttype == SpellHotbar::GameData::custom_transform_spell_type::fav_menu ||
                    casttype == SpellHotbar::GameData::custom_transform_spell_type::fav_menu_switch) {
                    slot_type = SpellHotbar::Storage::menu_slot_type::custom_favmenu;
                }
            }

            //logger::trace("Slotting spell {} on slot {}", form->GetFormID(), index);
            return SpellHotbar::Storage::slotSpell(form->GetFormID(), index, slot_type);
        }
    }
    return false;
}

bool is_transformed_with_fav_menu_binding(RE::StaticFunctionTag*) {
    if (SpellHotbar::GameData::isCustomTransform()) {
        auto type = SpellHotbar::GameData::getCustomTransformCasttype();
        return type == SpellHotbar::GameData::custom_transform_spell_type::fav_menu ||
            type == SpellHotbar::GameData::custom_transform_spell_type::fav_menu_switch;
    }
    return false;
}

void menu_move_bar_left(RE::StaticFunctionTag*)
{
    SpellHotbar::Bars::menu_bar_id = SpellHotbar::Bars::getPreviousMenuBar(SpellHotbar::Bars::menu_bar_id);
}

void menu_move_bar_right(RE::StaticFunctionTag*)
{
    SpellHotbar::Bars::menu_bar_id = SpellHotbar::Bars::getNextMenuBar(SpellHotbar::Bars::menu_bar_id);
}

void highlight_slot(RE::StaticFunctionTag*, int slot, bool error, float duration)
{
    SpellHotbar::RenderManager::highlight_skill_slot(slot, duration, error);
}

bool toggle_bar_enabled(RE::StaticFunctionTag*, int barID)
{
    bool is_enabled_after{false};
    if (SpellHotbar::Bars::hotbars.contains(barID)) {
        is_enabled_after = !SpellHotbar::Bars::hotbars.at(barID).is_enabled();
        SpellHotbar::Bars::hotbars.at(barID).set_enabled(is_enabled_after);
    } else {
        logger::warn("toggle_bar_enabled: Unknown bar id: {}", barID);
    }
    return is_enabled_after;
}
bool get_bar_enabled(RE::StaticFunctionTag*, int barID) {
    bool ret{false};
    if (SpellHotbar::Bars::hotbars.contains(barID)) {
        ret = SpellHotbar::Bars::hotbars.at(barID).is_enabled();
    } else {
        logger::warn("get_bar_enabled: Unknown bar id: {}", barID);
    }
    return ret;
}

int get_inherit_mode(RE::StaticFunctionTag*, int barID)
{
    if (SpellHotbar::Bars::hotbars.contains(barID))
    {
        return SpellHotbar::Bars::hotbars.at(barID).get_inherit_mode();
    }
    logger::warn("get_inherit_mode: Unknown bar id: {}", barID);
    return 0;
}

int set_inherit_mode(RE::StaticFunctionTag*, int barID, int inherit_mode)
{
    if (SpellHotbar::Bars::hotbars.contains(barID)) {
        return SpellHotbar::Bars::hotbars.at(barID).set_inherit_mode(inherit_mode);
    }
    logger::warn("set_inherit_mode: Unknown bar id: {}", barID);
    return 0;
}

int set_hud_bar_show_mode(RE::StaticFunctionTag*, int show_mode)
{
    SpellHotbar::Bars::bar_show_setting = SpellHotbar::Bars::bar_show_mode(std::clamp(show_mode, 0, 5));
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting);
}

int get_hud_bar_show_mode(RE::StaticFunctionTag*)
{
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting);
}

bool is_player_on_gcd(RE::StaticFunctionTag*)
{
    return !SpellHotbar::GameData::is_allowed_to_cast();
}

void set_player_gcd(RE::StaticFunctionTag*, float time)
{
    SpellHotbar::GameData::set_next_allowed_cast(time);
}

void reload_resources(RE::StaticFunctionTag*)
{ 
    SpellHotbar::RenderManager::reload_resouces();
}

void reload_spelldata(RE::StaticFunctionTag*)
{
    SpellHotbar::GameData::reload_data();
}

inline void _set_spell_casttime(RE::TESForm* form, float& out_casttime)
{
    if (form->GetFormType() == RE::FormType::Spell) {
        RE::SpellItem* spell = form->As<RE::SpellItem>();
        if (spell->GetChargeTime() > 0.01f) {
            out_casttime = spell->GetChargeTime();
        }
    }
}

float setup_cast_and_get_casttime(RE::StaticFunctionTag*, RE::TESForm* form)
{
    float casttime{0.01f};
    uint16_t anim_to_use{0Ui16};
    uint16_t cast_art{0Ui16};
    if (form) {
        RE::FormID id = form->GetFormID();

        if (SpellHotbar::GameData::spell_cast_info.contains(id)) {
            const auto& data = SpellHotbar::GameData::spell_cast_info.at(id);
            //logger::info("Anim to Use: {}", data.animation);
            if (data.casttime >= 0.01f) {
                casttime = data.casttime;
            } else {
                _set_spell_casttime(form, casttime);
            }
            anim_to_use = data.animation;
            cast_art = data.casteffectid;
        } else {
            _set_spell_casttime(form, casttime);
        }
    }

    SpellHotbar::GameData::update_spell_casting_art_and_time(cast_art, static_cast<uint32_t>(std::ceil(casttime+0.5)));

    if (SpellHotbar::GameData::global_animation_type) {
        SpellHotbar::GameData::global_animation_type->value = static_cast<float>(anim_to_use);
    }
    //logger::info("Returning Casttime: {}", casttime);
    return casttime;
}

void trigger_skill_cooldown(RE::StaticFunctionTag*, RE::TESForm* form)
{
    if (form) {
        RE::FormID id = form->GetFormID();

        if (SpellHotbar::GameData::spell_cast_info.contains(id)) {
            const auto& data = SpellHotbar::GameData::spell_cast_info.at(id);
            if (data.cooldown > 0.0f) {
                //logger::info("Trigger Cooldown {} : {}", form->GetName(), data.cooldown);
                SpellHotbar::GameData::add_gametime_cooldown_with_timescale(id, data.cooldown, true);
            }
        }
    }
}

bool is_player_on_cd_for_skill(RE::StaticFunctionTag*, RE::TESForm* form) {
    bool on_gcd = !SpellHotbar::GameData::is_allowed_to_cast();
    if (form && !on_gcd) {
        RE::FormID id = form->GetFormID();
        
        auto cal = RE::Calendar::GetSingleton();
        if (cal) {
            auto [prog, dur] = SpellHotbar::GameData::get_gametime_cooldown(cal->GetCurrentGameTime(), id);
            return dur > 0.0f;
        }
    }
    return on_gcd;
}


void show_drag_bar(RE::StaticFunctionTag*)
{
    SpellHotbar::RenderManager::start_bar_dragging();
}

int set_hud_bar_show_mode_vampire_lord(RE::StaticFunctionTag*, int show_mode) {
    SpellHotbar::Bars::bar_show_setting_vampire_lord = SpellHotbar::Bars::bar_show_mode(std::clamp(show_mode, 0, 2));
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_vampire_lord);
}

int get_hud_bar_show_mode_vampire_lord(RE::StaticFunctionTag*) {
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_vampire_lord);
}

int set_hud_bar_show_mode_werewolf(RE::StaticFunctionTag*, int show_mode) {
    SpellHotbar::Bars::bar_show_setting_werewolf = SpellHotbar::Bars::bar_show_mode(std::clamp(show_mode, 0, 2));
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_werewolf);
}

int get_hud_bar_show_mode_werewolf(RE::StaticFunctionTag*) {
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_werewolf);
}

bool save_bars_to_file(RE::StaticFunctionTag*, std::string filename)
{ 
    return SpellHotbar::Bars::save_bars_to_json(filename);
}

bool file_exists(RE::StaticFunctionTag*, std::string filepath)
{ 
    return std::filesystem::exists(filepath);
}

bool load_bars_from_file(RE::StaticFunctionTag*, std::string filename_mod_dir, std::string filename_user_dir)
{
    if (std::filesystem::exists(filename_user_dir)) {
        return SpellHotbar::Bars::load_bars_from_json(filename_user_dir);

    } else if (std::filesystem::exists(filename_mod_dir)) {
        return SpellHotbar::Bars::load_bars_from_json(filename_mod_dir);

    } else {
        logger::error("Could not load bars, both '{}' and '{}' do not exist!", filename_user_dir, filename_mod_dir);
        return false;
    }
}

void clear_bars(RE::StaticFunctionTag*)
{
    SpellHotbar::Bars::clear_bars();
}

float get_slot_spacing(RE::StaticFunctionTag*)
{
    return static_cast<float>(SpellHotbar::Bars::slot_spacing);
}

float set_slot_spacing(RE::StaticFunctionTag*, float spacing)
{
    SpellHotbar::Bars::slot_spacing = std::max(0, static_cast<int>(spacing));
    return static_cast<float>(SpellHotbar::Bars::slot_spacing);
}

int get_text_show_mode(RE::StaticFunctionTag*)
{ 
    return static_cast<int>(SpellHotbar::Bars::text_show_setting);
}

int set_text_show_mode(RE::StaticFunctionTag*, int value)
{ 
    SpellHotbar::Bars::text_show_setting = SpellHotbar::Bars::text_show_mode(static_cast<uint8_t>(std::clamp(value, 0, 2)));
    return static_cast<int>(SpellHotbar::Bars::text_show_setting);
}

bool is_default_bar_when_sheated(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::use_default_bar_when_sheathed;
}

bool toggle_default_bar_when_sheathed(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::use_default_bar_when_sheathed = !SpellHotbar::Bars::use_default_bar_when_sheathed;
}

bool is_disable_menu_rendering(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::disable_menu_rendering;
}

bool toggle_disable_menu_rendering(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::disable_menu_rendering = !SpellHotbar::Bars::disable_menu_rendering;
}

bool SpellHotbar::register_papyrus_functions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("getCurrentSelectedSpellInMenu", "SpellHotbar", get_current_selected_spell_in_menu);
    vm->RegisterFunction("getSlottedSpell", "SpellHotbar", get_slotted_spell);
    vm->RegisterFunction("slotSpell", "SpellHotbar", slot_spell);
    vm->RegisterFunction("isCtrlBarEnabled", "SpellHotbar", is_ctrl_enabled);
    vm->RegisterFunction("toggleCtrlBarEnabled", "SpellHotbar", toggle_ctrl_enabled);
    vm->RegisterFunction("isShiftBarEnabled", "SpellHotbar", is_shift_enabled);
    vm->RegisterFunction("toggleShiftBarEnabled", "SpellHotbar", toggle_shift_enabled);
    vm->RegisterFunction("isAltBarEnabled", "SpellHotbar", is_alt_enabled);
    vm->RegisterFunction("toggleAltBarEnabled", "SpellHotbar", toggle_alt_enabled);
    vm->RegisterFunction("getNumberOfSlots", "SpellHotbar", get_number_of_slots);
    vm->RegisterFunction("setNumberOfSlots", "SpellHotbar", set_number_of_slots);
    vm->RegisterFunction("setSlotScale", "SpellHotbar", set_slot_scale);
    vm->RegisterFunction("getSlotScale", "SpellHotbar", get_slot_scale);
    vm->RegisterFunction("setOffsetX", "SpellHotbar", set_offset_x);
    vm->RegisterFunction("getOffsetX", "SpellHotbar", get_offset_x);
    vm->RegisterFunction("setOffsetY", "SpellHotbar", set_offset_y);
    vm->RegisterFunction("getOffsetY", "SpellHotbar", get_offset_y);
    vm->RegisterFunction("bindMenuMoveBarLeft", "SpellHotbar", menu_move_bar_left);
    vm->RegisterFunction("bindMenuMoveBarRight", "SpellHotbar", menu_move_bar_right);
    vm->RegisterFunction("highlightSlot", "SpellHotbar", highlight_slot);
    vm->RegisterFunction("toggleBarEnabled", "SpellHotbar", toggle_bar_enabled);
    vm->RegisterFunction("getBarEnabled", "SpellHotbar", get_bar_enabled);
    vm->RegisterFunction("getInheritMode", "SpellHotbar", get_inherit_mode);
    vm->RegisterFunction("setInheritMode", "SpellHotbar", set_inherit_mode);
    vm->RegisterFunction("setHudBarShowMode", "SpellHotbar", set_hud_bar_show_mode);
    vm->RegisterFunction("getHudBarShowMode", "SpellHotbar", get_hud_bar_show_mode);
    vm->RegisterFunction("isPlayerOnGCD", "SpellHotbar", is_player_on_gcd);
    vm->RegisterFunction("setPlayerGCD", "SpellHotbar", set_player_gcd);
    vm->RegisterFunction("reloadResources", "SpellHotbar", reload_resources);
    vm->RegisterFunction("reloadData", "SpellHotbar", reload_spelldata);
    vm->RegisterFunction("setupCastAndGetCasttime", "SpellHotbar", setup_cast_and_get_casttime);
    vm->RegisterFunction("triggerSkillCooldown", "SpellHotbar", trigger_skill_cooldown);
    vm->RegisterFunction("isPlayerOnCD", "SpellHotbar", is_player_on_cd_for_skill);
    vm->RegisterFunction("showDragBar", "SpellHotbar", show_drag_bar);
    vm->RegisterFunction("setHudBarShowModeVampireLord", "SpellHotbar", set_hud_bar_show_mode_vampire_lord);
    vm->RegisterFunction("getHudBarShowModeVampireLord", "SpellHotbar", get_hud_bar_show_mode_vampire_lord);
    vm->RegisterFunction("setHudBarShowModeWerewolf", "SpellHotbar", set_hud_bar_show_mode_werewolf);
    vm->RegisterFunction("getHudBarShowModeWerewolf", "SpellHotbar", get_hud_bar_show_mode_werewolf);
    vm->RegisterFunction("saveBarsToFile", "SpellHotbar", save_bars_to_file);
    vm->RegisterFunction("fileExists", "SpellHotbar", file_exists);
    vm->RegisterFunction("loadBarsFromFile", "SpellHotbar", load_bars_from_file);
    vm->RegisterFunction("clearBars", "SpellHotbar", clear_bars);
    vm->RegisterFunction("setSlotSpacing", "SpellHotbar", set_slot_spacing);
    vm->RegisterFunction("getSlotSpacing", "SpellHotbar", get_slot_spacing);
    vm->RegisterFunction("getTextShowMode", "SpellHotbar", get_text_show_mode);
    vm->RegisterFunction("setTextShowMode", "SpellHotbar", set_text_show_mode);
    vm->RegisterFunction("isTransformedFavMenuBind", "SpellHotbar", is_transformed_with_fav_menu_binding);
    vm->RegisterFunction("isDefaultBarWhenSheathed", "SpellHotbar", is_default_bar_when_sheated);
    vm->RegisterFunction("toggleDefaultBarWhenSheathed", "SpellHotbar", toggle_default_bar_when_sheathed);
    vm->RegisterFunction("isDisableMenuRendering", "SpellHotbar", is_disable_menu_rendering);
    vm->RegisterFunction("toggleDisableMenuRendering", "SpellHotbar", toggle_disable_menu_rendering);
    return true;
}