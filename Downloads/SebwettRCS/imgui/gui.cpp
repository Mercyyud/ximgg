/*
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 * LEAKED BY DISCORD.GG/ENIGMALEAKS
 */

// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS
// LEAKED BY DISCORD.GG/ENIGMALEAKS

#include "gui.h"
#include <windows.h>
#include <cstdlib>
#include <cmath>

extern HWND hwnd;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "images/glow.c"

namespace icon1 {
#include "images/icon1.c"
}

namespace icon2 {
#include "images/icon2.c"
}

namespace player {
#include "images/player.c"
}

namespace username_img {
#include "images/username.c"
}

namespace password_img {
#include "images/password.c"
}

extern ID3D11Device* g_pd3dDevice;

ID3D11ShaderResourceView* glow_texture = nullptr;
int glow_width = 0;
int glow_height = 0;

ID3D11ShaderResourceView* icon1_texture = nullptr;
int icon1_w = 0, icon1_h = 0;
ID3D11ShaderResourceView* icon2_texture = nullptr;
int icon2_w = 0, icon2_h = 0;

ID3D11ShaderResourceView* player_texture = nullptr;
int player_w = 0, player_h = 0;

ID3D11ShaderResourceView* username_icon_texture = nullptr;
int username_icon_w = 0, username_icon_h = 0;
ID3D11ShaderResourceView* password_icon_texture = nullptr;
int password_icon_w = 0, password_icon_h = 0;

ImVec2 window::size = { 545, 450 };
float window::rounding = 7.0f;

ImFont* ui::font_title = nullptr;

bool LoadTextureFromMemory(const unsigned char* data, size_t data_size, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    int image_width = 0, image_height = 0, image_channels = 0;
    unsigned char* image_data = stbi_load_from_memory(data, (int)data_size, &image_width, &image_height, &image_channels, 4);
    if (!image_data) return false;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    if (FAILED(d3dDevice->CreateTexture2D(&desc, &subResource, &pTexture)))
    {
        stbi_image_free(image_data);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    if (FAILED(d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv)))
    {
        pTexture->Release();
        stbi_image_free(image_data);
        return false;
    }

    pTexture->Release();
    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

void ui::initialize()
{
    if (g_pd3dDevice != nullptr) {
        LoadTextureFromMemory(rawData, sizeof(rawData), g_pd3dDevice, &glow_texture, &glow_width, &glow_height);
        LoadTextureFromMemory(icon1::rawData, sizeof(icon1::rawData), g_pd3dDevice, &icon1_texture, &icon1_w, &icon1_h);
        LoadTextureFromMemory(icon2::rawData, sizeof(icon2::rawData), g_pd3dDevice, &icon2_texture, &icon2_w, &icon2_h);
        LoadTextureFromMemory(player::rawData, sizeof(player::rawData), g_pd3dDevice, &player_texture, &player_w, &player_h);
        LoadTextureFromMemory(username_img::rawData, sizeof(username_img::rawData), g_pd3dDevice, &username_icon_texture, &username_icon_w, &username_icon_h);
        LoadTextureFromMemory(password_img::rawData, sizeof(password_img::rawData), g_pd3dDevice, &password_icon_texture, &password_icon_w, &password_icon_h);
    }
}

void ui::render()
{
    ImGuiIO& io = ImGui::GetIO();

    static float expand_anim = 0.0f;
    static float expand_target = 0.0f;
    expand_anim += (expand_target - expand_anim) * fminf(1.0f, io.DeltaTime * 32.0f);

    const float expand_px_x = 100.0f;
    const float expand_px_y = 30.0f;
    const ImVec2 win_sz(window::size.x + expand_anim * expand_px_x, window::size.y + expand_anim * expand_px_y);
    const float win_round = window::rounding + expand_anim * 3.0f;
    const float content_img_alpha = 1.0f - expand_anim;
    const bool show_auth_ui = (expand_target < 0.001f);
    const bool show_dashboard = (expand_anim > 0.001f);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(win_sz);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, win_round);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGui::Begin("Base", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 pos = ImGui::GetWindowPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    static char user[64] = "";
    static char pass[64] = "";

    static float chrome_lift = 0.0f;
    const bool over_panel = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    chrome_lift += ((over_panel ? 1.0f : 0.0f) - chrome_lift) * fminf(1.0f, io.DeltaTime * 14.0f);

    const int glow_a = (int)(228.0f + 27.0f * chrome_lift);
    const int grid_a = (int)(165.0f + 90.0f * chrome_lift);

    draw->AddRectFilled(pos, ImVec2(pos.x + win_sz.x, pos.y + win_sz.y), IM_COL32(10, 11, 16, 255), win_round);

    if (glow_texture)
    {
        draw->AddImageRounded((void*)glow_texture, pos, ImVec2(pos.x + win_sz.x, pos.y + win_sz.y), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, glow_a), win_round);
    }

    float grid_step = 60.0f;
    ImU32 grid_color = IM_COL32(16, 17, 22, grid_a);
    for (float x = grid_step; x < win_sz.x; x += grid_step)
        draw->AddLine(ImVec2(pos.x + x, pos.y), ImVec2(pos.x + x, pos.y + win_sz.y), grid_color);
    for (float y = grid_step; y < win_sz.y; y += grid_step)
        draw->AddLine(ImVec2(pos.x, pos.y + y), ImVec2(pos.x + win_sz.x, pos.y + y), grid_color);

    float footer_height = 45.0f;

    static float player_lift = 0.0f;
    if (player_texture && content_img_alpha > 0.001f)
    {
        float display_h = win_sz.y - footer_height;
        float aspect = (float)player_w / (float)player_h;
        float display_w = display_h * aspect;

        ImVec2 p0(pos.x + 10.0f, pos.y + 5.0f);
        ImVec2 p1(pos.x + 10.0f + display_w, pos.y + 5.0f + display_h);
        const bool over_player = ImGui::IsMouseHoveringRect(p0, p1, false);
        player_lift += ((over_player ? 1.0f : 0.0f) - player_lift) * fminf(1.0f, io.DeltaTime * 16.0f);
        const int pa = (int)((232.0f + 23.0f * player_lift) * content_img_alpha);

        draw->AddImage((void*)player_texture, p0, p1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, pa));
    }

    float content_width = 240.0f;
    float start_x = win_sz.x - content_width - 40.0f;
    float start_y = 96.0f;

    ImGui::SetCursorPos(ImVec2(start_x, start_y));
    ImGui::BeginGroup();
    if (show_auth_ui)
    {
        ImGui::SetWindowFontScale(1.4f);
        const char* title_text = "Welcome Back,";
        ImVec2 text_pos = ImGui::GetCursorScreenPos();
        float total_width = ImGui::CalcTextSize(title_text).x;
        const float title_line_h = ImGui::GetTextLineHeight();
        const bool title_hover = ImGui::IsMouseHoveringRect(text_pos, ImVec2(text_pos.x + total_width, text_pos.y + title_line_h), false);
        static float title_lift = 0.0f;
        title_lift += ((title_hover ? 1.0f : 0.0f) - title_lift) * fminf(1.0f, io.DeltaTime * 18.0f);
        const float title_mul = 0.97f + 0.03f * title_lift;

        for (const char* c = title_text; *c != '\0'; ++c) {
            char buf[2] = { *c, '\0' };
            float current_x = ImGui::CalcTextSize(title_text, c).x;
            float t = current_x / total_width;

            ImU32 col = ImColor(0.0f, (0.60f + (0.15f - 0.60f) * t) * title_mul, (1.0f + (0.35f - 1.0f) * t) * title_mul, 1.0f);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(text_pos.x + current_x, text_pos.y), col, buf);
        }

        ImGui::SetCursorScreenPos(ImVec2(text_pos.x, text_pos.y + title_line_h + 6.0f));
        ImGui::SetWindowFontScale(1.0f);

        const char* sub_text = "Sign in to your account";
        ImVec2 sub_pos = ImGui::GetCursorScreenPos();
        ImVec2 sub_sz = ImGui::CalcTextSize(sub_text);
        const bool sub_hover = ImGui::IsMouseHoveringRect(sub_pos, ImVec2(sub_pos.x + sub_sz.x, sub_pos.y + sub_sz.y), false);
        static float sub_lift = 0.0f;
        sub_lift += ((sub_hover ? 1.0f : 0.0f) - sub_lift) * fminf(1.0f, io.DeltaTime * 18.0f);
        ImGui::TextColored(ImVec4(0.40f + 0.05f * sub_lift, 0.43f + 0.05f * sub_lift, 0.50f + 0.06f * sub_lift, 1.0f), "%s", sub_text);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 18.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 14));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 14));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.078f, 0.082f, 0.098f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.098f, 0.102f, 0.122f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.088f, 0.092f, 0.110f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.84f, 0.88f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.40f, 0.42f, 0.48f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.0f, 0.44f, 0.87f, 0.5f));

        const float field_icon = 22.0f;
        const float field_icon_pad = 10.0f;

        ImGui::SetNextItemWidth(content_width);
        ImGui::InputTextWithHint("##user", "Username", user, 64, ImGuiInputTextFlags_ElideLeft);
        {
            ImVec2 mn = ImGui::GetItemRectMin();
            ImVec2 mx = ImGui::GetItemRectMax();
            bool hovered = ImGui::IsItemHovered();
            static float user_icon_fade = 0.55f;
            float target = hovered ? 1.0f : 0.55f;
            user_icon_fade += (target - user_icon_fade) * ImGui::GetIO().DeltaTime * 18.0f;
            if (username_icon_texture && content_img_alpha > 0.001f) {
                float y = mn.y + (mx.y - mn.y - field_icon) * 0.5f;
                float x = mx.x - field_icon_pad - field_icon;
                ImU32 tint = IM_COL32(255, 255, 255, (int)(255.0f * user_icon_fade * content_img_alpha));
                draw->AddImage((void*)username_icon_texture, ImVec2(x, y), ImVec2(x + field_icon, y + field_icon), ImVec2(0, 0), ImVec2(1, 1), tint);
            }
        }

        ImGui::SetNextItemWidth(content_width);
        ImGui::InputTextWithHint("##pass", "Password", pass, 64, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_ElideLeft);
        {
            ImVec2 mn = ImGui::GetItemRectMin();
            ImVec2 mx = ImGui::GetItemRectMax();
            bool hovered = ImGui::IsItemHovered();
            static float pass_icon_fade = 0.55f;
            float target = hovered ? 1.0f : 0.55f;
            pass_icon_fade += (target - pass_icon_fade) * ImGui::GetIO().DeltaTime * 18.0f;
            if (password_icon_texture && content_img_alpha > 0.001f) {
                float y = mn.y + (mx.y - mn.y - field_icon) * 0.5f;
                float x = mx.x - field_icon_pad - field_icon;
                ImU32 tint = IM_COL32(255, 255, 255, (int)(255.0f * pass_icon_fade * content_img_alpha));
                draw->AddImage((void*)password_icon_texture, ImVec2(x, y), ImVec2(x + field_icon, y + field_icon), ImVec2(0, 0), ImVec2(1, 1), tint);
            }
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.92f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.46f, 0.98f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.38f, 0.85f, 1.0f));

        if (ImGui::Button("LOGIN", ImVec2(content_width, 42)))
            expand_target = 1.0f;

        ImGui::PopStyleColor(9);
        ImGui::PopStyleVar(4);
    }
    ImGui::EndGroup();

    if (show_dashboard)
    {
        float dash_alpha = expand_anim;
        static int active_tab = 1;
        ImU32 accent_blue = IM_COL32(0, 102, 255, (int)(255 * dash_alpha));
        
        // Large Wrapper Panel (matches the dark bordered container in screenshot)
        float pad = 20.0f;
        ImVec2 wp0 = ImVec2(pos.x + pad, pos.y + pad);
        ImVec2 wp1 = ImVec2(pos.x + win_sz.x - pad, pos.y + win_sz.y - pad);
        
        draw->AddRectFilled(wp0, wp1, IM_COL32(14, 15, 18, (int)(255 * dash_alpha)), 8.0f);
        draw->AddRect(wp0, wp1, IM_COL32(30, 31, 35, (int)(255 * dash_alpha)), 8.0f); // subtle outer border
        
        float start_x = wp0.x + 30.0f;
        float start_y = wp0.y + 35.0f;

        // Header Section
        ImGui::SetWindowFontScale(1.3f);
        draw->AddText(ImVec2(start_x, start_y), IM_COL32(255, 255, 255, (int)(255 * dash_alpha)), "Welcome back, ");
        float welcome_w = ImGui::CalcTextSize("Welcome back, ").x;
        
        char display_name[128];
        sprintf(display_name, "%s!", (user[0] != '\0' ? user : "exodus.dev"));
        ImVec2 name_sz = ImGui::CalcTextSize(display_name);
        ImVec2 n_pos = ImVec2(start_x + welcome_w, start_y);
        
        // Base blue
        draw->AddText(n_pos, accent_blue, display_name);
        
        float third = name_sz.x / 3.0f;
        // Mid: Dark blue
        draw->PushClipRect(ImVec2(n_pos.x + third, n_pos.y), ImVec2(n_pos.x + third*2.0f, n_pos.y + name_sz.y), true);
        draw->AddText(n_pos, IM_COL32(0, 60, 180, (int)(255 * dash_alpha)), display_name);
        draw->PopClipRect();

        // End: Black/dark grey
        draw->PushClipRect(ImVec2(n_pos.x + third*2.0f, n_pos.y), ImVec2(n_pos.x + name_sz.x, n_pos.y + name_sz.y), true);
        draw->AddText(n_pos, IM_COL32(30, 35, 45, (int)(255 * dash_alpha)), display_name);
        draw->PopClipRect();
        
        ImGui::SetWindowFontScale(1.0f);

        // TOP SECTION: TWO COLUMNS
        float col_spacing = 15.0f;
        float half_w = (wp1.x - wp0.x - 60.0f - col_spacing) * 0.5f;
        float cards_y = start_y + 45.0f;
        float cards_h = 95.0f;

        auto render_card_bg = [&](ImVec2 p0, ImVec2 p1) {
            // Hollow box look, no filled background
            draw->AddRect(p0, p1, IM_COL32(35, 36, 42, (int)(255 * dash_alpha)), 6.0f);
        };

        // COLUMN 1: MY PROFILE
        ImVec2 p0_1 = ImVec2(start_x, cards_y);
        ImVec2 p1_1 = ImVec2(start_x + half_w, cards_y + cards_h);
        render_card_bg(p0_1, p1_1);
        draw->AddText(ImVec2(p0_1.x + 15, p0_1.y + 12), accent_blue, "MY PROFILE");
        
        auto render_row = [&](ImVec2 start, const char* label, const char* val, float y, ImU32 val_col) {
            draw->AddText(ImVec2(start.x + 15, start.y + y), IM_COL32(110, 112, 120, (int)(255 * dash_alpha)), label);
            ImVec2 val_sz = ImGui::CalcTextSize(val);
            draw->AddText(ImVec2(start.x + half_w - 15 - val_sz.x, start.y + y), val_col, val);
        };

        render_row(p0_1, "Username", (user[0] != '\0' ? user : "exodus.dev"), 40, accent_blue);
        render_row(p0_1, "Expiry", "Lifetime", 60, accent_blue);
        render_row(p0_1, "Board", "MSI Pro", 80, IM_COL32(230, 230, 235, (int)(255 * dash_alpha)));

        // COLUMN 2: USER INFORMATION
        ImVec2 p0_2 = ImVec2(start_x + half_w + col_spacing, cards_y);
        ImVec2 p1_2 = ImVec2(p0_2.x + half_w, cards_y + cards_h);
        render_card_bg(p0_2, p1_2);
        draw->AddText(ImVec2(p0_2.x + 15, p0_2.y + 12), accent_blue, "USER INFORMATION");

        render_row(p0_2, "TPM State", "Enabled", 40, IM_COL32(70, 210, 70, (int)(255 * dash_alpha)));
        render_row(p0_2, "Sec Boot", "Disabled", 60, IM_COL32(230, 70, 70, (int)(255 * dash_alpha)));
        render_row(p0_2, "Build", "4.1 Private", 80, accent_blue);

        // BOTTOM SECTION: ANNOUNCEMENTS
        float ann_y = cards_y + cards_h + 15.0f;
        float ann_h = 105.0f;
        ImVec2 ap0 = ImVec2(start_x, ann_y);
        ImVec2 ap1 = ImVec2(p1_2.x, ann_y + ann_h); // fill exactly to end of column 2
        render_card_bg(ap0, ap1);
        draw->AddText(ImVec2(ap0.x + 15, ap0.y + 12), accent_blue, "ANNOUNCEMENTS");

        auto render_ann = [&](const char* title, const char* desc, float y) {
            draw->AddRectFilled(ImVec2(ap0.x + 15, ap0.y + y), ImVec2(ap0.x + 17, ap0.y + y + 30), accent_blue, 1.0f);
            draw->AddText(ImVec2(ap0.x + 25, ap0.y + y), IM_COL32(230, 230, 235, (int)(255 * dash_alpha)), title);
            draw->AddText(ImVec2(ap0.x + 25, ap0.y + y + 16), IM_COL32(110, 112, 120, (int)(255 * dash_alpha)), desc);
        };

        render_ann("System update 4.1 -- 2026/03/28", "Improved NIC Spoofing & Advanced TPM bypass added.", 40);
        render_ann("Security patch 4.0 -- 2026/03/25", "Enhanced driver mapping and Windows 11 compatibility.", 75);

        // Footer Tabs (aligned perfectly inside the main wrapper)
        float footer_y = wp1.y - 35.0f; 
        auto render_footer_tab = [&](int idx, const char* label, float x_off) {
            bool active = (active_tab == idx);
            ImVec2 n_pos = ImVec2(start_x + x_off, footer_y);
            ImVec2 t_sz = ImGui::CalcTextSize(label);
            bool hovered = ImGui::IsMouseHoveringRect(ImVec2(n_pos.x - 10, n_pos.y - 5), ImVec2(n_pos.x + t_sz.x + 10, n_pos.y + 20));
            if (hovered && ImGui::IsMouseClicked(0)) active_tab = idx;
            
            draw->AddText(n_pos, active ? accent_blue : IM_COL32(110, 112, 120, (int)(255 * dash_alpha)), label);
            if (active) draw->AddRectFilled(ImVec2(n_pos.x, n_pos.y + 18), ImVec2(n_pos.x + t_sz.x, n_pos.y + 20), accent_blue, 1.0f);
        };

        render_footer_tab(0, "Home", 0);
        render_footer_tab(1, "Spoofer", 60);
        render_footer_tab(2, "Serials", 140);
    }

    
    ImGui::SetCursorPos(ImVec2(0, win_sz.y - footer_height));
    ImGui::BeginChild("Footer", ImVec2(win_sz.x, footer_height), false, ImGuiWindowFlags_NoBackground);
    ImVec2 f_pos = ImGui::GetWindowPos(); // Transparent footer instead of dark background block so tabs float freely

    float icon_size = 20.0f;
    float padding_x = 25.0f;
    float spacing = 12.0f;
    float y_pos = (footer_height - icon_size) * 0.5f;

    float x_right = win_sz.x - padding_x - icon_size;
    ImGui::SetCursorPos(ImVec2(x_right, y_pos));
    ImGui::InvisibleButton("DiscordIcon", ImVec2(icon_size, icon_size));
    static float a1 = 0.6f;
    a1 += ((ImGui::IsItemHovered() ? 1.0f : 0.6f) - a1) * ImGui::GetIO().DeltaTime * 16.0f;
    if (icon1_texture) draw->AddImage((void*)icon1_texture, ImVec2(f_pos.x + x_right, f_pos.y + y_pos), ImVec2(f_pos.x + x_right + icon_size, f_pos.y + y_pos + icon_size), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, (int)(255 * a1)));

    float x_left = x_right - spacing - icon_size;
    ImGui::SetCursorPos(ImVec2(x_left, y_pos));
    ImGui::InvisibleButton("EarthIcon", ImVec2(icon_size, icon_size));
    static float a2 = 0.6f;
    a2 += ((ImGui::IsItemHovered() ? 1.0f : 0.6f) - a2) * ImGui::GetIO().DeltaTime * 16.0f;
    if (icon2_texture) draw->AddImage((void*)icon2_texture, ImVec2(f_pos.x + x_left, f_pos.y + y_pos), ImVec2(f_pos.x + x_left + icon_size, f_pos.y + y_pos + icon_size), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, (int)(255 * a2)));

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    if (hwnd) {
        const int nw = (int)floorf(win_sz.x + 0.5f);
        const int nh = (int)floorf(win_sz.y + 0.5f);
        RECT wr;
        if (GetWindowRect(hwnd, &wr)) {
            const int cw = wr.right - wr.left;
            const int ch = wr.bottom - wr.top;
            if (cw != nw || ch != nh) {
                const int dx = (nw - cw) / 2;
                const int dy = (nh - ch) / 2;
                SetWindowPos(hwnd, nullptr, wr.left - dx, wr.top - dy, nw, nh, SWP_NOZORDER);
            }
        }
    }
}

void ui::shutdown()
{
    if (glow_texture) { glow_texture->Release(); glow_texture = nullptr; }
    if (icon1_texture) { icon1_texture->Release(); icon1_texture = nullptr; }
    if (icon2_texture) { icon2_texture->Release(); icon2_texture = nullptr; }
    if (player_texture) { player_texture->Release(); player_texture = nullptr; }
    if (username_icon_texture) { username_icon_texture->Release(); username_icon_texture = nullptr; }
    if (password_icon_texture) { password_icon_texture->Release(); password_icon_texture = nullptr; }
}