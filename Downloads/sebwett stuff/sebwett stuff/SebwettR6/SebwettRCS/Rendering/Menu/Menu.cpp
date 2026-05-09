/*
 * xim.gg RCS - Vertical Accordion Menu
 *
 * Controls:
 *   Up / Down       Move through visible rows
 *   Left / Right    Tune recoil values when X/Y is highlighted
 *   Enter           Toggle global enable or operator active state
 *   Tab             Switch between attackers and defenders
 *   Insert          Open / close menu (handled by Application)
 */

#include "Menu.h"

#include "../../Core/Config/Config.h"
#include "../../Core/Input/Hotkey.h"
#include "../../Core/Operators/OperatorData.h"
#include "../../Core/Recoil/RecoilEngine.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <imgui.h>
#include <string>
#include <vector>

namespace Menu {

static constexpr ImVec4 C_BG        = { 0.035f, 0.038f, 0.045f, 0.98f };
static constexpr ImVec4 C_PANEL     = { 0.055f, 0.060f, 0.072f, 1.00f };
static constexpr ImVec4 C_HEADER    = { 0.075f, 0.084f, 0.100f, 1.00f };
static constexpr ImVec4 C_ROW       = { 0.045f, 0.050f, 0.062f, 1.00f };
static constexpr ImVec4 C_SELECTED  = { 0.115f, 0.185f, 0.245f, 1.00f };
static constexpr ImVec4 C_TEXT      = { 0.925f, 0.930f, 0.935f, 1.00f };
static constexpr ImVec4 C_DIM       = { 0.565f, 0.590f, 0.625f, 1.00f };
static constexpr ImVec4 C_LINE      = { 0.175f, 0.195f, 0.225f, 1.00f };
static constexpr ImVec4 C_ACCENT    = { 0.300f, 0.680f, 0.980f, 1.00f };
static constexpr ImVec4 C_ON        = { 0.180f, 0.820f, 0.500f, 1.00f };
static constexpr ImVec4 C_OFF       = { 0.850f, 0.280f, 0.330f, 1.00f };

static constexpr float MENU_W = 318.0f;
static constexpr float MENU_X = 20.0f;
static constexpr float MENU_Y = 60.0f;
static constexpr float ROW_H  = 31.0f;
static constexpr int   MAX_VISIBLE_OPERATOR_ROWS = 16;
static constexpr float RECOIL_STEP = 0.5f;

enum class RowType {
    Enable,
    Operator,
    RecoilX,
    RecoilY
};

struct MenuRow {
    RowType type;
    const OperatorData::Operator* op;
};

static Config::Settings              s_Settings;
static Config::Profile               s_Profile;
static const OperatorData::Operator* s_ActiveOp = nullptr;
static int                           s_Page = 0;       // 0 attackers, 1 defenders
static int                           s_CursorRow = 1;  // 0 is global enable
static bool                          s_NeedsSave = false;
static float                         s_SaveTimer = 0.0f;

struct RepeatKey {
    bool down = false;
    float held = 0.0f;
    float nextRepeat = 0.0f;
};

static RepeatKey s_KeyUp;
static RepeatKey s_KeyDown;
static RepeatKey s_KeyLeft;
static RepeatKey s_KeyRight;
static RepeatKey s_KeyEnter;
static RepeatKey s_KeyTab;

static bool IsAnyDown(std::initializer_list<int> keys) {
    for (int key : keys) {
        if ((GetAsyncKeyState(key) & 0x8000) != 0)
            return true;
    }
    return false;
}

static bool KeyPressedAny(std::initializer_list<int> keys, RepeatKey& state) {
    bool current = IsAnyDown(keys);
    bool edge = current && !state.down;
    state.down = current;
    return edge;
}

static bool KeyRepeatAny(std::initializer_list<int> keys, RepeatKey& state, float dt, float firstDelay, float baseRate) {
    bool current = IsAnyDown(keys);
    bool fire = false;

    if (current) {
        if (!state.down) {
            state.held = 0.0f;
            state.nextRepeat = firstDelay;
            fire = true;
        } else {
            state.held += dt;
            if (state.held >= state.nextRepeat) {
                fire = true;
                float rate = state.held > 1.2f ? baseRate * 0.45f :
                             state.held > 0.6f ? baseRate * 0.65f :
                             baseRate;
                state.nextRepeat = state.held + rate;
            }
        }
    } else {
        state.held = 0.0f;
        state.nextRepeat = 0.0f;
    }

    state.down = current;
    return fire;
}

static void PushMenuStyle() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 4.0f;
    st.FrameRounding = 3.0f;
    st.WindowPadding = { 0.0f, 0.0f };
    st.ItemSpacing = { 0.0f, 0.0f };
    st.FramePadding = { 8.0f, 4.0f };
    st.ScrollbarSize = 4.0f;
    st.WindowBorderSize = 1.0f;
    st.ChildBorderSize = 0.0f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = C_BG;
    c[ImGuiCol_Border] = C_LINE;
    c[ImGuiCol_Text] = C_TEXT;
    c[ImGuiCol_TextDisabled] = C_DIM;
    c[ImGuiCol_ChildBg] = C_PANEL;
    c[ImGuiCol_ScrollbarBg] = { 0.0f, 0.0f, 0.0f, 0.0f };
    c[ImGuiCol_ScrollbarGrab] = { 0.22f, 0.25f, 0.30f, 0.60f };
}

static void ScheduleSave() {
    s_NeedsSave = true;
    s_SaveTimer = 0.45f;
}

static std::vector<const OperatorData::Operator*> CurrentList() {
    return (s_Page == 0) ? OperatorData::GetAttackers()
                         : OperatorData::GetDefenders();
}

static void ApplyProfile() {
    if (s_ActiveOp) {
        RecoilEngine::SetParams(
            s_Profile.recoilX,
            s_Profile.recoilY,
            s_Settings.onlyADS,
            s_Settings.onlyR6);
    } else {
        RecoilEngine::SetParams(0.0f, 0.0f, s_Settings.onlyADS, s_Settings.onlyR6);
    }
}

static void ActivateOperator(const OperatorData::Operator* op) {
    if (!op) return;

    s_ActiveOp = op;
    s_Profile = Config::LoadProfile(op->name);
    s_Profile.operatorName = op->name;
    s_Settings.lastOperator = op->name;
    ApplyProfile();
    ScheduleSave();
}

static void DeactivateOperator() {
    if (s_ActiveOp)
        Config::SaveProfile(s_Profile);

    s_ActiveOp = nullptr;
    ApplyProfile();
    ScheduleSave();
}

static std::vector<MenuRow> BuildRows() {
    std::vector<MenuRow> rows;
    rows.push_back({ RowType::Enable, nullptr });

    for (const OperatorData::Operator* op : CurrentList()) {
        rows.push_back({ RowType::Operator, op });

        if (s_ActiveOp && op == s_ActiveOp) {
            rows.push_back({ RowType::RecoilX, op });
            rows.push_back({ RowType::RecoilY, op });
        }
    }

    return rows;
}

static void ClampCursor(const std::vector<MenuRow>& rows) {
    if (rows.empty()) {
        s_CursorRow = 0;
        return;
    }

    if (s_CursorRow < 0)
        s_CursorRow = (int)rows.size() - 1;
    else if (s_CursorRow >= (int)rows.size())
        s_CursorRow = 0;
}

static void KeepCursorOnSameOperator(const OperatorData::Operator* target) {
    if (!target) {
        s_CursorRow = 1;
        return;
    }

    std::vector<MenuRow> rows = BuildRows();
    for (int i = 0; i < (int)rows.size(); ++i) {
        if (rows[i].type == RowType::Operator && rows[i].op == target) {
            s_CursorRow = i;
            return;
        }
    }

    s_CursorRow = 1;
}

static void DrawSeparator(ImDrawList* dl, float w) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(p, { p.x + w, p.y + 1.0f }, ImGui::ColorConvertFloat4ToU32(C_LINE));
    ImGui::Dummy({ 0.0f, 1.0f });
}

static void DrawRowBackground(ImDrawList* dl, const ImVec2& min, float w, float h, bool selected) {
    dl->AddRectFilled(
        min,
        { min.x + w, min.y + h },
        ImGui::ColorConvertFloat4ToU32(selected ? C_SELECTED : C_ROW));

    if (selected) {
        dl->AddRectFilled(
            min,
            { min.x + 3.0f, min.y + h },
            ImGui::ColorConvertFloat4ToU32(C_ACCENT));
    }
}

static void DrawTextRight(ImDrawList* dl, const ImVec2& min, float w, float h, const char* text, ImU32 color) {
    ImVec2 size = ImGui::CalcTextSize(text);
    dl->AddText(
        { min.x + w - size.x - 12.0f, min.y + (h - size.y) * 0.5f },
        color,
        text);
}

static void DrawToggleBox(ImDrawList* dl, const ImVec2& min, float h, bool enabled, bool selected) {
    float size = 16.0f;
    float x = min.x + 14.0f;
    float y = min.y + (h - size) * 0.5f;
    ImU32 border = ImGui::ColorConvertFloat4ToU32(enabled ? C_ON : C_DIM);
    ImU32 fill = enabled ? IM_COL32(18, 95, 68, 235) : IM_COL32(18, 21, 26, 235);

    if (enabled || selected) {
        float pulse = 0.5f + 0.5f * sinf((float)ImGui::GetTime() * 5.0f);
        int alpha = enabled ? (int)(32.0f + pulse * 34.0f) : 28;
        dl->AddRectFilled(
            { x - 4.0f, y - 4.0f },
            { x + size + 4.0f, y + size + 4.0f },
            IM_COL32(48, 174, 136, alpha),
            6.0f);
    }

    dl->AddRectFilled({ x, y }, { x + size, y + size }, fill, 4.0f);
    dl->AddRect({ x, y }, { x + size, y + size }, border, 4.0f, 0, 1.4f);

    if (enabled) {
        dl->AddLine({ x + 4.0f, y + 8.0f }, { x + 7.0f, y + 11.0f }, IM_COL32(225, 255, 245, 255), 2.0f);
        dl->AddLine({ x + 7.0f, y + 11.0f }, { x + 12.5f, y + 5.0f }, IM_COL32(225, 255, 245, 255), 2.0f);
    }
}

static void DrawCenteredText(ImDrawList* dl, const ImVec2& min, float w, float h, const char* text, ImU32 color) {
    ImVec2 size = ImGui::CalcTextSize(text);
    dl->AddText(
        { min.x + (w - size.x) * 0.5f, min.y + (h - size.y) * 0.5f },
        color,
        text);
}

static void ToggleCurrentRow(const MenuRow& row) {
    if (row.type == RowType::Enable) {
        s_Settings.recoilEnabled = !s_Settings.recoilEnabled;
        RecoilEngine::SetEnabled(s_Settings.recoilEnabled);
        ScheduleSave();
        return;
    }

    if (row.type == RowType::Operator) {
        if (s_ActiveOp == row.op)
            DeactivateOperator();
        else
            ActivateOperator(row.op);
    }
}

static void TuneCurrentRow(const MenuRow& row, float delta) {
    if (!s_ActiveOp || row.op != s_ActiveOp)
        return;

    if (row.type == RowType::RecoilX) {
        s_Profile.recoilX = std::clamp(s_Profile.recoilX + delta, -50.0f, 50.0f);
    } else if (row.type == RowType::RecoilY) {
        s_Profile.recoilY = std::clamp(s_Profile.recoilY + delta, 0.0f, 100.0f);
    } else {
        return;
    }

    ApplyProfile();
    ScheduleSave();
}

static void HandleKeyboard(std::vector<MenuRow>& rows, float dt) {
    bool upPressed = KeyRepeatAny({ VK_UP }, s_KeyUp, dt, 0.26f, 0.085f);
    bool downPressed = KeyRepeatAny({ VK_DOWN }, s_KeyDown, dt, 0.26f, 0.085f);
    bool leftPressed = KeyRepeatAny({ VK_LEFT }, s_KeyLeft, dt, 0.20f, 0.045f);
    bool rightPressed = KeyRepeatAny({ VK_RIGHT }, s_KeyRight, dt, 0.20f, 0.045f);
    bool enterPressed = KeyPressedAny({ VK_RETURN }, s_KeyEnter);
    bool tabPressed = KeyPressedAny({ VK_TAB }, s_KeyTab);

    if (tabPressed) {
        const OperatorData::Operator* keep = s_ActiveOp;
        s_Page = (s_Page == 0) ? 1 : 0;
        KeepCursorOnSameOperator((keep && keep->side == (s_Page == 0 ? OperatorData::ATTACKER : OperatorData::DEFENDER)) ? keep : nullptr);
        rows = BuildRows();
        ClampCursor(rows);
    }

    if (upPressed) {
        --s_CursorRow;
        ClampCursor(rows);
    }

    if (downPressed) {
        ++s_CursorRow;
        ClampCursor(rows);
    }

    if (rows.empty())
        return;

    MenuRow row = rows[s_CursorRow];

    float tuneStep = RECOIL_STEP;
    if (s_KeyLeft.held > 1.2f || s_KeyRight.held > 1.2f)
        tuneStep = RECOIL_STEP * 3.0f;
    else if (s_KeyLeft.held > 0.6f || s_KeyRight.held > 0.6f)
        tuneStep = RECOIL_STEP * 2.0f;

    if (leftPressed)
        TuneCurrentRow(row, -tuneStep);
    if (rightPressed)
        TuneCurrentRow(row, tuneStep);
    if (enterPressed) {
        ToggleCurrentRow(row);
        rows = BuildRows();
        ClampCursor(rows);
    }
}

void Initialize() {
    PushMenuStyle();

    s_Settings = Config::LoadSettings();
    RecoilEngine::SetEnabled(s_Settings.recoilEnabled);

    const OperatorData::Operator* op = nullptr;
    if (!s_Settings.lastOperator.empty())
        op = OperatorData::Find(s_Settings.lastOperator);

    if (op) {
        s_Page = (op->side == OperatorData::DEFENDER) ? 1 : 0;
        ActivateOperator(op);
        KeepCursorOnSameOperator(op);
    } else {
        auto attackers = OperatorData::GetAttackers();
        if (!attackers.empty())
            s_CursorRow = 1;
        ApplyProfile();
    }
}

void Render() {
    PushMenuStyle();

    float dt = ImGui::GetIO().DeltaTime;
    if (s_NeedsSave) {
        s_SaveTimer -= dt;
        if (s_SaveTimer <= 0.0f) {
            Config::SaveSettings(s_Settings);
            if (s_ActiveOp)
                Config::SaveProfile(s_Profile);
            s_NeedsSave = false;
        }
    }

    std::vector<MenuRow> rows = BuildRows();
    ClampCursor(rows);
    HandleKeyboard(rows, dt);

    const int listRowCount = std::max(0, (int)rows.size() - 1);
    const float visibleRows = (float)std::min(listRowCount, MAX_VISIBLE_OPERATOR_ROWS);
    const float listH = ROW_H * visibleRows;
    ImGui::SetNextWindowPos({ MENU_X, MENU_Y }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ MENU_W, 0.0f }, ImGuiCond_Always);

    ImGui::Begin("##XimGGRCSMenu", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SetWindowFontScale(1.10f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = MENU_W;

    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = 42.0f;
        dl->AddRectFilled(p, { p.x + w, p.y + h }, ImGui::ColorConvertFloat4ToU32(C_HEADER));
        dl->AddRectFilled({ p.x, p.y }, { p.x + w, p.y + 2.0f }, ImGui::ColorConvertFloat4ToU32(C_ACCENT));

        dl->AddText({ p.x + 14.0f, p.y + 13.0f }, ImGui::ColorConvertFloat4ToU32(C_TEXT), "xim.gg RCS");
        DrawTextRight(dl, p, w, h, "[ v1.2 ]", ImGui::ColorConvertFloat4ToU32(C_DIM));
        ImGui::Dummy({ w, h });
    }

    DrawSeparator(dl, w);

    {
        bool selected = (s_CursorRow == 0);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = ROW_H + 6.0f;
        DrawRowBackground(dl, p, w, h, selected);

        DrawToggleBox(dl, p, h, s_Settings.recoilEnabled, selected);
        dl->AddText({ p.x + 40.0f, p.y + (h - ImGui::GetTextLineHeight()) * 0.5f },
            ImGui::ColorConvertFloat4ToU32(C_TEXT), "ENABLE");
        DrawTextRight(dl, p, w, h, s_Settings.recoilEnabled ? "ON" : "OFF",
            ImGui::ColorConvertFloat4ToU32(s_Settings.recoilEnabled ? C_ON : C_OFF));

        ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton("##enable", { w, h });
        if (ImGui::IsItemClicked()) {
            s_CursorRow = 0;
            ToggleCurrentRow({ RowType::Enable, nullptr });
        }
    }

    DrawSeparator(dl, w);

    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = 42.0f;
        dl->AddRectFilled(p, { p.x + w, p.y + h }, ImGui::ColorConvertFloat4ToU32(C_PANEL));

        float pad = 12.0f;
        float segX = p.x + pad;
        float segY = p.y + 7.0f;
        float segW = w - pad * 2.0f;
        float segH = h - 14.0f;
        float half = segW * 0.5f;
        float activeX = segX + (s_Page == 0 ? 0.0f : half);

        dl->AddRectFilled({ segX, segY }, { segX + segW, segY + segH }, IM_COL32(14, 18, 23, 220), 6.0f);
        dl->AddRect({ segX, segY }, { segX + segW, segY + segH }, ImGui::ColorConvertFloat4ToU32(C_LINE), 6.0f);
        dl->AddRectFilled({ activeX + 2.0f, segY + 2.0f }, { activeX + half - 2.0f, segY + segH - 2.0f },
            ImGui::ColorConvertFloat4ToU32(C_SELECTED), 5.0f);
        dl->AddRectFilled({ activeX + 12.0f, segY + segH - 4.0f }, { activeX + half - 12.0f, segY + segH - 2.0f },
            ImGui::ColorConvertFloat4ToU32(C_ACCENT), 2.0f);

        DrawCenteredText(dl, { segX, segY }, half, segH, "ATTACKERS",
            ImGui::ColorConvertFloat4ToU32(s_Page == 0 ? C_ACCENT : C_DIM));
        DrawCenteredText(dl, { segX + half, segY }, half, segH, "DEFENDERS",
            ImGui::ColorConvertFloat4ToU32(s_Page == 1 ? C_ACCENT : C_DIM));

        ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton("##side-tabs", { w, h });
        if (ImGui::IsItemClicked()) {
            float localX = ImGui::GetIO().MousePos.x - segX;
            int clickedPage = (localX >= half) ? 1 : 0;
            if (localX >= 0.0f && localX <= segW && s_Page != clickedPage) {
                s_Page = clickedPage;
                s_CursorRow = 1;
            }
        }
    }

    DrawSeparator(dl, w);

    ImGui::BeginChild("##accordionRows", { w, listH }, false, ImGuiWindowFlags_NoScrollbar);

    float scrollY = ImGui::GetScrollY();
    float cursorTop = (float)std::max(0, s_CursorRow - 1) * ROW_H;
    if (cursorTop < scrollY)
        ImGui::SetScrollY(cursorTop);
    else if (cursorTop + ROW_H > scrollY + listH)
        ImGui::SetScrollY(cursorTop + ROW_H - listH);

    for (int i = 1; i < (int)rows.size(); ++i) {
        const MenuRow& row = rows[i];
        bool selected = (i == s_CursorRow);
        bool active = (row.op && s_ActiveOp == row.op);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* childDl = ImGui::GetWindowDrawList();

        DrawRowBackground(childDl, p, w, ROW_H, selected);

        if (row.type == RowType::Operator) {
            DrawToggleBox(childDl, p, ROW_H, active, selected);
            childDl->AddText(
                { p.x + 40.0f, p.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f },
                ImGui::ColorConvertFloat4ToU32(active ? C_TEXT : C_DIM),
                row.op->name.c_str());

            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton(row.op->name.c_str(), { w, ROW_H });
            if (ImGui::IsItemClicked()) {
                s_CursorRow = i;
                ToggleCurrentRow(row);
                rows = BuildRows();
            }
        } else {
            float value = (row.type == RowType::RecoilX) ? s_Profile.recoilX : s_Profile.recoilY;
            const char* label = (row.type == RowType::RecoilX) ? "X-REC" : "Y-REC";
            char valueText[32] = {};
            sprintf_s(valueText, "%.1f", value);

            float indentX = p.x + 40.0f;
            float valueW = selected ? 88.0f : 74.0f;
            float valueH = 21.0f;
            float valueX = p.x + w - valueW - 12.0f;
            float valueY = p.y + (ROW_H - valueH) * 0.5f;

            if (selected) {
                childDl->AddRectFilled({ p.x + 30.0f, p.y + 8.0f }, { p.x + 33.0f, p.y + ROW_H - 8.0f },
                    ImGui::ColorConvertFloat4ToU32(C_ACCENT), 2.0f);
            }

            childDl->AddText(
                { indentX, p.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f },
                ImGui::ColorConvertFloat4ToU32(selected ? C_TEXT : C_DIM),
                label);
            childDl->AddRectFilled({ valueX, valueY }, { valueX + valueW, valueY + valueH },
                IM_COL32(18, 22, 28, 230), 5.0f);
            childDl->AddRect({ valueX, valueY }, { valueX + valueW, valueY + valueH },
                selected ? ImGui::ColorConvertFloat4ToU32(C_ACCENT) : ImGui::ColorConvertFloat4ToU32(C_LINE), 5.0f);

            if (selected) {
                float cy = valueY + valueH * 0.5f;
                ImU32 arrowCol = ImGui::ColorConvertFloat4ToU32(C_ACCENT);
                childDl->AddTriangleFilled(
                    { valueX + 10.0f, cy },
                    { valueX + 16.0f, cy - 5.0f },
                    { valueX + 16.0f, cy + 5.0f },
                    arrowCol);
                childDl->AddTriangleFilled(
                    { valueX + valueW - 10.0f, cy },
                    { valueX + valueW - 16.0f, cy - 5.0f },
                    { valueX + valueW - 16.0f, cy + 5.0f },
                    arrowCol);
            }

            DrawCenteredText(childDl, { valueX + (selected ? 16.0f : 0.0f), valueY },
                valueW - (selected ? 32.0f : 0.0f), valueH, valueText,
                ImGui::ColorConvertFloat4ToU32(C_TEXT));

            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton(row.type == RowType::RecoilX ? "##xrec" : "##yrec", { w, ROW_H });
            if (ImGui::IsItemClicked()) {
                s_CursorRow = i;
            }
        }
    }

    ImGui::EndChild();

    ImGui::End();
}

} // namespace Menu
