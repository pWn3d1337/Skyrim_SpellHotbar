#include "render_manager.h"
#include "texture_loader.h"
#include "../bar/hotbars.h"
#include "../bar/hotbar.h"

#include "../logger/logger.h"
#include "../storage/storage.h"

#include <d3d11.h>
#include <d3d11_3.h>

#include <dxgi.h>
#include "../game_data/game_data.h"
#include "texture_csv_loader.h"
#include <unordered_map>
#include "../input/input.h"

#include <imgui_internal.h>

#define IMAGES_ROOT_PATH ".\\data\\SKSE\\Plugins\\SpellHotbar\\images\\"

// Hook render stuff for imgui, mostly copied from wheeler
namespace stl {
    using namespace SKSE::stl;

    template <class T>
    void write_thunk_call() {
        auto& trampoline = SKSE::GetTrampoline();
        const REL::Relocation<std::uintptr_t> hook{T::id, T::offset};
        T::func = trampoline.write_call<5>(hook.address(), T::thunk);
    }
}

namespace SpellHotbar {

bool TextureImage::load(const std::string& path)
{
    return TextureLoader::fromFile(path.c_str(), &res, &width, &height);
}

void TextureImage::draw(float w, float h)
{
    ImGui::Image((void*)res, ImVec2(w, h));
}

void TextureImage::draw(float w, float h, float alpha)
{
    ImGui::Image((void*)res, ImVec2(w, h), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0, 1.0, 1.0, alpha));
}

void TextureImage::draw_on_top(ImVec2 pos, float w, float h, ImU32 col) {
    ImGui::GetWindowDrawList()->AddImage((void*)res, pos, ImVec2(pos.x + w, pos.y + h), ImVec2(0.0,0.0), ImVec2(1.0,1.0), col);
}

void TextureImage::draw()
{
    draw(static_cast<float>(width), static_cast<float>(height));
}

SubTextureImage::SubTextureImage(const TextureImage& other, ImVec2 uv0, ImVec2 uv1)
    :
    TextureImage(),
    uv0(uv0),
    uv1(uv1)
{
    this->res = other.res;
    this->width = other.width;
    this->height = other.height;
}

void SubTextureImage::draw(float w, float h)
{
    ImGui::Image((void*)res, ImVec2(w, h), uv0, uv1); }

void SubTextureImage::draw(float w, float h, float alpha)
{ 
    ImGui::Image((void*)res, ImVec2(w, h), uv0, uv1, ImVec4(1.0f, 1.0f, 1.0f, alpha));
}

void SubTextureImage::draw_with_scale(float w, float h, float alpha, float scale) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));

    float dx = ((w * scale) - w) * 0.5f;
    float dy = ((h * scale) - h) * 0.5f;

    ImGui::GetWindowDrawList()->AddImage((void*)res, ImVec2(pos.x - dx, pos.y - dy), ImVec2(pos.x + w + dx, pos.y + h + dy), uv0, uv1,
                                         ImColor(1.0f, 1.0f, 1.0f, alpha));
}

void SubTextureImage::draw_on_top(ImVec2 pos, float w, float h, ImU32 col)
{
    ImGui::GetWindowDrawList()->AddImage((void*)res, pos, ImVec2(pos.x + w, pos.y + h), uv0, uv1, col);
}

ImFont* font_text = nullptr;
ImFont* font_symbols = nullptr;

float font_text_size{0};

std::vector<TextureImage> loaded_textures;
std::unordered_map<RE::FormID, SubTextureImage> spell_icons;
std::unordered_map<GameData::DefaultIconType, SubTextureImage> default_icons;
std::vector<SubTextureImage> cooldown_icons;

int highlight_slot = -1;
float highlight_time = 0.0F;
float highlight_dur_total = 1.0F;
bool highlight_isred = false;

bool menu_open = false;

bool show_drag_frame = false;
bool drag_frame_initialized = false;
ImVec2 drag_window_pos = ImVec2(0,0);
float drag_window_width = 0.0f;
ImVec2 drag_window_start_pos = ImVec2(0, 0);
float drag_window_start_width = 0;

void update_highlight(float delta) {
    if (highlight_time > 0.0F) {
        highlight_time -= delta;
        if (highlight_time < 0.0F) {
            highlight_time = 0.0F;
            highlight_slot = -1;
            highlight_isred = false;
        }
    }
}

float get_highlight_factor()
{
    return highlight_time / highlight_dur_total;
}

void RenderManager::highlight_skill_slot(int id, float dur, bool error)
{ 
    highlight_slot = id;
    highlight_dur_total = dur;
    highlight_time = dur;
    highlight_isred = error;
}

enum class fade_type
{
    none,
    fade_in,
    fade_out
};

//Bar fade timers
float hud_fade_timer {1.0f};
float hud_fade_time_max {1.0f};
fade_type hud_fade_type {fade_type::none};
bool last_hud_should_show {false};

void finish_fade()
{ 
    hud_fade_timer = 1.0f;
    hud_fade_time_max = 1.0f;
    hud_fade_type = fade_type::none;
}

void update_fade_timer(float delta)
{ 
    switch (hud_fade_type) {
        case fade_type::fade_in:
            hud_fade_timer += delta;
            if (hud_fade_timer >= hud_fade_time_max) {
                finish_fade();
            }
            break;
        case fade_type::fade_out:
            hud_fade_timer -= delta;
            if (hud_fade_timer <= 0.0f) {
                finish_fade();
            }
            break;
    default:
        break;
    }
}

void start_fade_in(float duration)
{
    float new_val{0.0f};
    if (hud_fade_type == fade_type::fade_out) {
        float current_prog = hud_fade_timer / hud_fade_time_max;
        new_val = duration * current_prog;
    }

    hud_fade_type = fade_type::fade_in;
    hud_fade_timer = new_val;
    hud_fade_time_max = duration;
}

void start_fade_out(float duration) {
    float new_val{duration};
    if (hud_fade_type == fade_type::fade_in) {
        float current_prog = hud_fade_timer / hud_fade_time_max;
        new_val *= current_prog;
    }

    hud_fade_type = fade_type::fade_out;
    hud_fade_timer = new_val;
    hud_fade_time_max = duration;
}

bool is_hud_fading()
{ 
    return hud_fade_type != fade_type::none;
}

float RenderManager::get_bar_alpha()
{
    return hud_fade_type == fade_type::none ? 1.0f : hud_fade_timer / hud_fade_time_max;
}

void RenderManager::start_bar_dragging()
{ 
    show_drag_frame = true;
    drag_frame_initialized = false;

    drag_window_pos = ImVec2(0, 0);
    drag_window_width = 0.0f;
    drag_window_start_pos = ImVec2(0, 0);
    drag_window_start_width = 0;
}

bool RenderManager::should_block_game_cursor_inputs() { return show_drag_frame; }

void RenderManager::stop_bar_dragging()
{ 
    show_drag_frame = false;
}

// text fade timers
float text_fade_timer{1.0f};
float text_fade_time_max{1.0f};
fade_type text_fade_type{fade_type::none};

uint32_t last_bar{0};

void start_text_fade(float time)
{
    text_fade_timer = time;
    text_fade_time_max = time;
}

void text_fade_check_last_bar(uint32_t bar) {
    if (last_bar != bar) {
        start_text_fade(2.5f);
    }
    last_bar = bar;
}

void update_text_fade_timer(float delta) {
    if (text_fade_timer > delta) {
        text_fade_timer -= delta;
    } else {
        text_fade_timer = 0.0f;
    }
}

float get_text_fade_alpha() {
    if (Bars::text_show_setting == Bars::text_show_mode::never) {
        return 0.0f;
    } else if (Bars::text_show_setting == Bars::text_show_mode::always){
        return 1.0f;
    } else {
        if (text_fade_timer > 0.0f) {
            if (text_fade_timer <= (text_fade_time_max / 3.0f)) {
                    // Last third -> fade out
                    return text_fade_timer / (text_fade_time_max / 3.0f);

            } else if (text_fade_timer >= (text_fade_time_max * 2.0f / 3.0f)) {
                    // First third -> fade in
                    float f_max = (text_fade_time_max * 2.0f / 3.0f);
                    float f = text_fade_timer - f_max;
                    return 1.0f - (f / f_max);
            } else {
                    return 1.0f;  // middle third -> fully opaque
            }

        } else {
            return 0.0f;
        }
    }
}


template <typename T>
void _check_ptr(T* ptr, std::string name) {
    if (ptr == nullptr) {
        logger::error("Error loading {}", name);
    }
}
#define CHECK_PTR(a) _check_ptr(a, #a) //this passes both as var and string(name of var)

void load_font_resources(float window_height) {
    //calculate required font size, defaults are for 1080p
    float scale_factor = window_height / 1080.0f;

    font_text_size = std::roundf(24.0f * scale_factor);
    float size_symbols = std::roundf(36.0f * scale_factor); 

    logger::info("Loading Fonts with sizes {}, {}", font_text_size, size_symbols);

    ImGuiIO& io = ImGui::GetIO();
    font_text = io.Fonts->AddFontFromFileTTF(
        ".\\data\\SKSE\\Plugins\\SpellHotbar\\fonts\\9_$ConsoleFont_Futura Condensed.ttf", font_text_size);

    font_symbols = io.Fonts->AddFontFromFileTTF(
        ".\\data\\SKSE\\Plugins\\SpellHotbar\\fonts\\2_$SkyrimSymbolsFont_SkyrimSymbols.ttf",
       size_symbols);

    CHECK_PTR(font_text);
    CHECK_PTR(font_symbols);
}

void RenderManager::load_gamedata_dependant_resources() {
    TextureCSVLoader::load_icons(std::filesystem::path(IMAGES_ROOT_PATH));
}

void RenderManager::reload_resouces() {
    //Handled
    // std::vector<TextureImage> loaded_textures;
    // std::unordered_map<RE::FormID, SubTextureImage> spell_icons;
    // std::unordered_map<GameData::DefaultIconType, SubTextureImage> default_icons;
    // std::vector<SubTextureImage> cooldown_icons;

    //TODO fonts, reloading fonts in imgui is not that simple
    logger::info("Clearing all Resources...");

    spell_icons.clear();
    default_icons.clear();
    cooldown_icons.clear();

    for (const auto& teximg : loaded_textures) {
        teximg.res->Release();
    }
    loaded_textures.clear();

    logger::info("Reloading Resources...");
    RenderManager::load_gamedata_dependant_resources();
}

TextureImage & RenderManager::load_texture(std::string path) {
    TextureImage tex_img;

    if (tex_img.load(path)) {
        loaded_textures.push_back(std::move(tex_img));
    }
    return loaded_textures.back();
}

void RenderManager::add_spell_texture(TextureImage& main_texture, RE::FormID formID, ImVec2 uv0, ImVec2 uv1) {
    SubTextureImage tex_img(main_texture, uv0, uv1);
    auto it = spell_icons.find(formID);
    if (it != spell_icons.end()) {
        it->second = std::move(tex_img);
    } else {
        spell_icons.insert(std::make_pair(formID, std::move(tex_img)));
    }
}

void RenderManager::add_default_icon(TextureImage& main_texture, GameData::DefaultIconType type, ImVec2 uv0, ImVec2 uv1)
{
    SubTextureImage tex_img(main_texture, uv0, uv1);
    auto it = default_icons.find(type);
    if (it != default_icons.end()) {
        it->second = std::move(tex_img);
    } else {
        default_icons.insert(std::make_pair(type, std::move(tex_img)));
    }
}

void RenderManager::add_cooldown_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1)
{
    cooldown_icons.emplace_back(main_texture, uv0, uv1);
}

void RenderManager::init_cooldown_icons(size_t amount) {
    cooldown_icons.clear();
    cooldown_icons.reserve(amount);
}

LRESULT RenderManager::WndProcHook::thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto& io = ImGui::GetIO();
    if (uMsg == WM_KILLFOCUS) {
        io.ClearInputCharacters();
        io.ClearInputKeys();
    }

    return func(hWnd, uMsg, wParam, lParam);
}

void RenderManager::D3DInitHook::thunk() {
    func();

    logger::info("RenderManager: Initializing...");
    auto renderer = RE::BSGraphics::Renderer::GetSingleton();
    if (!renderer){
        logger::error("Cannot find renderer. Initialization failed!");
        return;
    }
    auto render_data = renderer->GetRendererData();
    if (!render_data) {
        logger::error("Cannot get renderer data. Initialization failed!");
        return;
    }

    logger::info("Getting swapchain...");
    auto render_window = renderer->GetCurrentRenderWindow();
    if (!render_window) {
        logger::error("Cannot get render_window. Initialization failed!");
        return;
    }
    auto swapchain = render_window->swapChain;
    if (!swapchain) {
        logger::error("Cannot get swapChain. Initialization failed!");
        return;
    }

    logger::info("Getting swapchain desc...");
    DXGI_SWAP_CHAIN_DESC sd{};
    if (swapchain->GetDesc(std::addressof(sd)) < 0) {
        logger::error("IDXGISwapChain::GetDesc failed.");
        return;
    }

    device = render_data->forwarder;
    context = render_data->context;

    logger::info("Initializing ImGui...");
    ImGui::CreateContext();

    //auto& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    if (!ImGui_ImplWin32_Init(sd.OutputWindow)) {
        logger::error("ImGui initialization failed (Win32)");
        return;
    } else {
        ImGui_ImplWin32_EnableAlphaCompositing(sd.OutputWindow);
    }
    if (!ImGui_ImplDX11_Init(device, context)) {
        logger::error("ImGui initialization failed (DX11)");
        return;
    }

    logger::info("...ImGui Initialized");

    initialized.store(true);

    WndProcHook::func = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(sd.OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook::thunk)));
    if (!WndProcHook::func) logger::error("SetWindowLongPtrA failed!");

    logger::trace("Loading fonts...");
    //get window size for font from HWND since imgui does not know size yet
    RECT rect;
    if (GetWindowRect(sd.OutputWindow, &rect)) {
        int height = rect.bottom - rect.top;
        load_font_resources(static_cast<float>(height));
    } else {
        logger::error("Could not get window size for font loading");
    }

    logger::info("RenderManager: Initialized");
}

void RenderManager::DXGIPresentHook::thunk(std::uint32_t a_p1) {
    func(a_p1);

    if (!D3DInitHook::initialized.load()) return;

    // start imgui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // My stuff
    RenderManager::draw();

    // end imgui
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void RenderManager::MessageCallback(SKSE::MessagingInterface::Message* msg)
{
    if (msg->type == SKSE::MessagingInterface::kDataLoaded && D3DInitHook::initialized) {
        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        io.WantSetMousePos = false;
    }
}

bool RenderManager::install() {
    auto g_message = SKSE::GetMessagingInterface();
    if (!g_message) {
        logger::error("Messaging Interface Not Found!");
        return false;
    }

    g_message->RegisterListener(MessageCallback);

    SKSE::AllocTrampoline(14 * 2);

    stl::write_thunk_call<D3DInitHook>();
    stl::write_thunk_call<DXGIPresentHook>();

    return true;
}

void RenderManager::spell_slotted_draw_anim(int index)
{ 
    highlight_time = highlight_dur_total;
    highlight_slot = index;
}

void RenderManager::draw_bg(int size, float alpha)
{
    if (default_icons.contains(GameData::DefaultIconType::BAR_EMPTY)) {
        auto& sub_image = default_icons.at(GameData::DefaultIconType::BAR_EMPTY);
        sub_image.draw(static_cast<float>(size), static_cast<float>(size), alpha);
    } else {
        ImGui::Dummy(ImVec2(static_cast<float>(size), static_cast<float>(size)));
    }
}

bool RenderManager::draw_skill(RE::FormID formID, int size, float alpha) {
    constexpr float scale = 1.0f;
    if (formID != 0) {
    
        if (GameData::is_clear_spell(formID) && default_icons.contains(GameData::DefaultIconType::UNBIND_SLOT)) {
            auto& clear_img = default_icons.at(GameData::DefaultIconType::UNBIND_SLOT);
            clear_img.draw_with_scale(static_cast<float>(size), static_cast<float>(size), alpha, scale);
            return true;
        }
        else if (spell_icons.contains(formID)) {
            auto& img = spell_icons.at(formID);
            img.draw_with_scale(static_cast<float>(size), static_cast<float>(size), alpha, scale);

            return true;
        } else {
            // fallback icons
            auto form = RE::TESForm::LookupByID(formID);
            GameData::DefaultIconType icon{GameData::DefaultIconType::UNKNOWN};
            if (form != nullptr) {
                icon = GameData::get_fallback_icon_type(form);
            }

            if (default_icons.contains(icon)) {
                auto& sub_image = default_icons.at(icon);
                sub_image.draw_with_scale(static_cast<float>(size), static_cast<float>(size), alpha, scale);
                return true;
            } else {
                if (default_icons.contains(GameData::DefaultIconType::UNKNOWN)) {
                    auto& unk_image = default_icons.at(GameData::DefaultIconType::UNKNOWN);
                    unk_image.draw_with_scale(static_cast<float>(size), static_cast<float>(size), alpha, scale);
                    return true;
                }
            }
        }
    }
    return false;
}

void RenderManager::draw_slot_overlay(ImVec2 pos, int size, ImU32 col)
{
    if (default_icons.contains(GameData::DefaultIconType::BAR_OVERLAY)) {
        auto& overlay = default_icons.at(GameData::DefaultIconType::BAR_OVERLAY);
        overlay.draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    } 
}

void RenderManager::draw_cd_overlay(ImVec2 pos, int size, float cd, ImU32 col) {
    if (cooldown_icons.size() > 0) {
        size_t i = static_cast<size_t>(std::round((cooldown_icons.size()-1) * cd));
        size_t index = std::clamp(i, static_cast<size_t>(0U), static_cast<size_t>(cooldown_icons.size()-1U));
        cooldown_icons.at(index).draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    }
}

void RenderManager::draw_highlight_overlay(ImVec2 pos, int size, ImU32 col)
{
    if (default_icons.contains(GameData::DefaultIconType::BAR_HIGHLIGHT)) {
        auto& overlay = default_icons.at(GameData::DefaultIconType::BAR_HIGHLIGHT);
        overlay.draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    }
}

void RenderManager::draw_scaled_text(ImVec2 pos, ImU32 col, const char* text)
{
    float size = (Bars::slot_scale +0.25f)* font_text_size;
    ImGui::GetWindowDrawList()->AddText(font_text, size, pos, col, text);
}

void TextCenterHorizontal(std::string text) {
    float font_size = ImGui::GetFontSize() * text.size() / 2;
    ImGui::SameLine(ImGui::GetWindowSize().x / 2 - font_size + (font_size / 2));
    ImGui::Text(text.c_str());
}

void drawCenteredText(std::string text, float alpha = 1.0F) {
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, alpha), text.c_str());
}

inline bool is_ultrawide(const float & screen_size_x, const float & screen_size_y) {
    //16:9 is ~1.777, 21:9 is ~2.333
    return screen_size_x / screen_size_y >= 2.0f;
}

/**
* Return screen_size_x, screen_size_y, window_width
*/
inline std::tuple<float, float, float> calculate_menu_window_size()
{
    //use default spacing in menu
    ImGui::GetStyle().ItemSpacing = ImVec2(8, 4);

    const float screen_size_x = ImGui::GetIO().DisplaySize.x, screen_size_y = ImGui::GetIO().DisplaySize.y;

    float icons_height =
        (get_slot_height(screen_size_y) + ImGui::GetStyle().ItemSpacing.y) * static_cast<float>(Bars::barsize);

    ImGui::PushFont(font_text);
    float text_height_offset = ImGui::CalcTextSize("M").y + ImGui::GetStyle().ItemSpacing.y * 6;
    ImGui::PopFont();

    float window_height = icons_height + text_height_offset;
    float window_width {0.0f};
    if (is_ultrawide(screen_size_x, screen_size_y)) {
        window_width = screen_size_x * 0.35f;
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5f, 0.0f));
    } else {
        window_width = screen_size_x * 0.3f;
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5755f, 0.0f));
    }
    ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
    ImGui::SetNextWindowBgAlpha(0.65F);
    return std::make_tuple(screen_size_x, screen_size_y, window_width);
}

/*
* return screen_size_x, screen_size_y, window_width
*/
inline std::tuple<float, float,float> calculate_hud_window_size()
{
    auto& io = ImGui::GetIO();
    const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    spacing.x = static_cast<float>(Bars::slot_spacing);
    ImGui::GetStyle().ItemSpacing = spacing;

    ImVec2 inner_spacing = ImGui::GetStyle().ItemInnerSpacing;

    float slot_h = std::floor(get_hud_slot_height(screen_size_y));
    ImGui::PushFont(font_text);
    float window_height = slot_h + ImGui::CalcTextSize("M").y + spacing.y * 2 + inner_spacing.y *2;
    ImGui::PopFont();

    float window_width = (slot_h + spacing.x) * static_cast<float>(Bars::barsize-1) + slot_h + inner_spacing.x *4;

    ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
    return std::make_tuple(screen_size_x, screen_size_y, window_width);
}

void draw_drag_menu() {
    static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse;

    auto& io = ImGui::GetIO();
    io.MouseDrawCursor = true;

    auto [screen_size_x, screen_size_y, window_width] = calculate_hud_window_size();
    if (!drag_frame_initialized) {
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5f - window_width * 0.5f + Bars::offset_x,
                                        screen_size_y*0.8f + Bars::offset_y)); //- slot_h * 2.5f
    }
    ImGui::SetNextWindowBgAlpha(0.65F);

    float alpha = 0.5f;
    SpellHotbar::Hotbar dummy("Drag Position, Mouse Wheel: Scale, ALT + Mouse Wheel: Spacing");
    ImGui::Begin("Drag Position, Mouse Wheel: Scale, ALT + Mouse Wheel: Spacing", &show_drag_frame, window_flag);

    drag_window_pos = ImGui::GetWindowPos();
    drag_window_width = ImGui::GetWindowWidth();
    if (!drag_frame_initialized) {
        drag_window_start_pos = drag_window_pos;
        drag_window_start_width = drag_window_width;
        drag_frame_initialized = true;
    }

    ImGui::SetItemUsingMouseWheel();    

    float constexpr scale_diff = 0.05f;

    //if (ImGui::IsItemHovered()) {
    if (io.MouseWheel < 0) {
        if (Input::mod_alt.isDown()) {
            if (Bars::slot_spacing > 0) {
                Bars::slot_spacing--;
            }
        } else {
            if (Bars::slot_scale > 0.1f) {
                Bars::slot_scale -= scale_diff;
            }
        }
    } else if (io.MouseWheel > 0) {
        if (Input::mod_alt.isDown()) {
            if (Bars::slot_spacing < 50) {
                Bars::slot_spacing++;
            }
        } else {
            if (Bars::slot_scale < 10.0f) {
                Bars::slot_scale += scale_diff;
            }
        }
    }

    dummy.draw_in_hud(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), key_modifier::none,
                    highlight_isred, alpha, 0.0f, 0);

}

//Draw Custom stuff 
void RenderManager::draw() {
    float deltaTime = ImGui::GetIO().DeltaTime;
    update_highlight(deltaTime);
    update_fade_timer(deltaTime);
    update_text_fade_timer(deltaTime);

    auto pc = RE::PlayerCharacter::GetSingleton();
    if (!pc || !pc->Is3DLoaded()) {
        return;  // no player, no draw
    }

    // begin draw
    auto ui = RE::UI::GetSingleton();
    if (!ui) {
        return;  // no ui reference, no draw
    }
    if (!ui->GameIsPaused()) {
        GameData::update_gcd_timer(deltaTime);
    }

    // check for ctrl/shift/alt modifiers
    key_modifier mod = Bars::get_current_modifier();

    auto* magMenu = static_cast<RE::MagicMenu*>(ui->GetMenu(RE::MagicMenu::MENU_NAME).get());
    auto* favMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(RE::FavoritesMenu::MENU_NAME).get());

    if (show_drag_frame) {
        // show frame to drag the bar
        draw_drag_menu();
        ImGui::End();
    }
    else
    {
        if (drag_frame_initialized) {
            //update bar position after drag has finished
            float width_diff = (drag_window_width - drag_window_start_width) * 0.5f;

            Bars::offset_x += (drag_window_pos.x - drag_window_start_pos.x) + width_diff;
            Bars::offset_y += (drag_window_pos.y - drag_window_start_pos.y);
            drag_window_pos = {0, 0};
            drag_window_start_pos = {0, 0};
            drag_window_width = 0.0f;
            drag_window_start_width = 0.0f;

            drag_frame_initialized = false;
        }

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        //io.WantCaptureMouse = false;

        if (magMenu) {

            if (!menu_open) {
                // menu was open first time
                menu_open = true;
                Bars::menu_bar_id = Bars::getCurrentHotbar_ingame();
            }
            if (!Bars::disable_menu_rendering) {

                // draw hotbar
                static constexpr ImGuiWindowFlags window_flag =
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;  // ImGuiWindowFlags_NoBackground

                auto [screen_size_x, screen_size_y, window_width] = calculate_menu_window_size();

                ImGui::Begin("SpellHotbar", nullptr, window_flag);
                if (Bars::hotbars.contains(Bars::menu_bar_id)) {
                    auto& bar = Bars::hotbars.at(Bars::menu_bar_id);

                    auto& prev_bar = Bars::hotbars.at(SpellHotbar::Bars::getPreviousMenuBar(SpellHotbar::Bars::menu_bar_id));
                    auto& next_bar = Bars::hotbars.at(SpellHotbar::Bars::getNextMenuBar(SpellHotbar::Bars::menu_bar_id));

                    ImGui::PushFont(font_symbols);
                    ImGui::Text("6");
                    ImGui::PopFont();
                    ImGui::SameLine();

                    bool table_ok = ImGui::BeginTable("SpellHotbarNavigation", 3, 0, ImVec2(window_width * 0.85f, 0.0f));
                    if (table_ok) {
                        ImGui::TableNextColumn();

                        ImGui::PushFont(font_text);
                        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImColor(0, 0, 0, 0).Value);
                        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImColor(192, 192, 192).Value);

                        ImGui::Button(prev_bar.get_name().c_str(), ImVec2(-FLT_MIN, 0.0f));
                        ImGui::TableNextColumn();

                        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImColor(255, 255, 255).Value);
                        ImGui::Button(bar.get_name().c_str(), ImVec2(-FLT_MIN, 0.0f));
                        ImGui::PopStyleColor();
                        ImGui::TableNextColumn();

                        ImGui::Button(next_bar.get_name().c_str(), ImVec2(-FLT_MIN, 0.0f));

                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                        ImGui::PopFont();
                        ImGui::EndTable();

                    }
                    else {
                        logger::trace("Error Rendering Table");
                    }
                    ImGui::SameLine();
                    ImGui::PushFont(font_symbols);
                    ImGui::Text("7");
                    ImGui::PopFont();

                    bar.draw_in_menu(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), mod);
                }
                else {
                    logger::error("Unknown Bar: {}", Bars::menu_bar_id);
                }
                ImGui::End();
            }

        } else if (favMenu && GameData::hasFavMenuSlotBinding()) {
            // draw vampire lord / werewolf bind menu
            if (!Bars::disable_menu_rendering) {
                uint32_t bar_id = Bars::getCurrentHotbar_ingame();
                if (Bars::hotbars.contains(bar_id)) {
                    auto& bar = Bars::hotbars.at(bar_id);

                    static constexpr ImGuiWindowFlags window_flag =
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;  // ImGuiWindowFlags_NoBackground

                    auto [screen_size_x, screen_size_y, window_width] = calculate_menu_window_size();

                    ImGui::Begin("SpellHotbar", nullptr, window_flag);

                    bar.draw_in_menu(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), mod);

                    ImGui::End();
                }
            }
        } else {
            menu_open = false;

            auto [should_show, fade_dur] = GameData::shouldShowHUDBar();
            if (!should_show && last_hud_should_show) {
                start_fade_out(fade_dur);
            } else if (should_show && !last_hud_should_show) {
                start_fade_in(fade_dur);
            }
            last_hud_should_show = should_show;

            if (should_show || is_hud_fading()) {
                auto* hudMenu = static_cast<RE::HUDMenu*>(ui->GetMenu(RE::HUDMenu::MENU_NAME).get());
                float shout_cd_prog = 1.0f;
                float shout_cd_dur = 0.0;
                if (hudMenu) {
                    shout_cd_prog = std::clamp(hudMenu->GetRuntimeData().shout->GetFillPct() * 0.01f, 0.0f, 1.0f);
                    shout_cd_dur = hudMenu->GetRuntimeData().shout->cooldown;
                }

                static constexpr ImGuiWindowFlags window_flag =
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

                auto [screen_size_x, screen_size_y, window_width] = calculate_hud_window_size();

                ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5f - window_width * 0.5f + Bars::offset_x,
                                               screen_size_y * 0.8f + Bars::offset_y)); //- slot_h * 2.5f 
                ImGui::SetNextWindowBgAlpha(0.65F);

                ImGui::Begin("SpellHotbarHUD", nullptr, window_flag);

                uint32_t bar_id = Bars::getCurrentHotbar_ingame();
                if (Bars::hotbars.contains(bar_id)) {
                    text_fade_check_last_bar(bar_id);

                    float alpha = get_bar_alpha();
                    float text_alpha = get_text_fade_alpha();
                    auto& bar = Bars::hotbars.at(bar_id);
                    ImGui::PushFont(font_text);
                    drawCenteredText(bar.get_name(), alpha* text_alpha);
                    ImGui::PopFont();
                    bar.draw_in_hud(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), mod,
                                    highlight_isred, alpha, shout_cd_prog, shout_cd_dur);
                } else {
                    logger::error("Unknown Bar: {}", bar_id);
                }

                ImGui::End();
            }
        }
    }
}
}