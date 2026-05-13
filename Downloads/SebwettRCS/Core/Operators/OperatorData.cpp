#include "OperatorData.h"
#include <algorithm>

namespace OperatorData {

static std::vector<Operator> s_Ops;
static bool s_Init = false;

static void Init() {
    if (s_Init) return;
    s_Init = true;

    // ── ATTACKERS ──────────────────────────────────────────────────────────
    s_Ops.push_back({ "Ace",          ATTACKER,  0.0f,  6.0f });
    s_Ops.push_back({ "Amaru",        ATTACKER,  0.0f,  4.5f });
    s_Ops.push_back({ "Ash",          ATTACKER,  0.0f,  7.0f });
    s_Ops.push_back({ "Blackbeard",   ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Blitz",        ATTACKER,  0.0f,  0.0f });
    s_Ops.push_back({ "Brava",        ATTACKER,  0.0f,  6.0f });
    s_Ops.push_back({ "Buck",         ATTACKER,  0.0f,  7.5f });
    s_Ops.push_back({ "Capitao",      ATTACKER,  0.0f,  6.0f });
    s_Ops.push_back({ "Deimos",       ATTACKER,  0.0f,  5.5f });
    s_Ops.push_back({ "Dokkaebi",     ATTACKER,  0.0f,  4.5f });
    s_Ops.push_back({ "Finka",        ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Flores",       ATTACKER,  0.0f,  5.5f });
    s_Ops.push_back({ "Fuze",         ATTACKER,  0.0f,  6.0f });
    s_Ops.push_back({ "Glaz",         ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Gridlock",     ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Grim",         ATTACKER,  0.0f,  5.5f });
    s_Ops.push_back({ "Hibana",       ATTACKER,  0.0f,  6.5f });
    s_Ops.push_back({ "Iana",         ATTACKER,  0.0f,  4.5f });
    s_Ops.push_back({ "IQ",           ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Jackal",       ATTACKER,  0.0f,  5.5f });
    s_Ops.push_back({ "Kali",         ATTACKER,  0.0f, 10.0f });
    s_Ops.push_back({ "Lion",         ATTACKER,  0.0f,  6.0f });
    s_Ops.push_back({ "Maverick",     ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Montagne",     ATTACKER,  0.0f,  0.0f });
    s_Ops.push_back({ "Nokk",         ATTACKER,  0.0f,  4.0f });
    s_Ops.push_back({ "Nomad",        ATTACKER,  0.0f,  5.5f });
    s_Ops.push_back({ "Osa",          ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Ram",          ATTACKER,  0.0f,  7.0f });
    s_Ops.push_back({ "Rauora",       ATTACKER,  0.0f,  4.5f });
    s_Ops.push_back({ "Sens",         ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Sledge",       ATTACKER,  0.0f,  4.5f });
    s_Ops.push_back({ "Solid Snake",  ATTACKER,  0.0f,  8.0f });
    s_Ops.push_back({ "Striker",      ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Thatcher",     ATTACKER,  0.0f,  4.5f });
    s_Ops.push_back({ "Thermite",     ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Twitch",       ATTACKER,  0.0f,  8.0f });
    s_Ops.push_back({ "Ying",         ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Zero",         ATTACKER,  0.0f,  5.0f });
    s_Ops.push_back({ "Zofia",        ATTACKER,  0.0f,  6.5f });

    // ── DEFENDERS ─────────────────────────────────────────────────────────
    s_Ops.push_back({ "Alibi",        DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Aruni",        DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Azami",        DEFENDER,  0.0f,  4.5f });
    s_Ops.push_back({ "Bandit",       DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Castle",       DEFENDER,  0.0f,  3.5f });
    s_Ops.push_back({ "Caveira",      DEFENDER,  0.0f,  3.0f });
    s_Ops.push_back({ "Clash",        DEFENDER,  0.0f,  0.0f });
    s_Ops.push_back({ "Denari",       DEFENDER,  0.0f,  6.0f });
    s_Ops.push_back({ "Doc",          DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Echo",         DEFENDER,  0.0f,  3.5f });
    s_Ops.push_back({ "Ela",          DEFENDER,  0.0f,  6.0f });
    s_Ops.push_back({ "Fenrir",       DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Frost",        DEFENDER,  0.0f,  3.5f });
    s_Ops.push_back({ "Goyo",         DEFENDER,  0.0f,  4.5f });
    s_Ops.push_back({ "Jager",        DEFENDER,  0.0f,  5.5f });
    s_Ops.push_back({ "Kaid",         DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Kapkan",       DEFENDER,  0.0f,  4.5f });
    s_Ops.push_back({ "Lesion",       DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Maestro",      DEFENDER,  0.0f,  5.0f });
    s_Ops.push_back({ "Melusi",       DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Mira",         DEFENDER,  0.0f,  4.5f });
    s_Ops.push_back({ "Mozzie",       DEFENDER,  0.0f,  5.0f });
    s_Ops.push_back({ "Mute",         DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Oryx",         DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Pulse",        DEFENDER,  0.0f,  3.5f });
    s_Ops.push_back({ "Rook",         DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Sentry",       DEFENDER,  0.0f,  5.0f });
    s_Ops.push_back({ "Skopos",       DEFENDER,  0.0f,  5.0f });
    s_Ops.push_back({ "Smoke",        DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Solis",        DEFENDER,  0.0f,  3.5f });
    s_Ops.push_back({ "Tachanka",     DEFENDER,  0.0f,  4.5f });
    s_Ops.push_back({ "Thorn",        DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Thunderbird",  DEFENDER,  0.0f,  5.0f });
    s_Ops.push_back({ "Tubarao",      DEFENDER,  0.0f,  3.5f });
    s_Ops.push_back({ "Valkyrie",     DEFENDER,  0.0f,  3.5f });
    s_Ops.push_back({ "Vigil",        DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Wamai",        DEFENDER,  0.0f,  4.0f });
    s_Ops.push_back({ "Warden",       DEFENDER,  0.0f,  3.5f });

    // Sort alphabetically within each side
    std::sort(s_Ops.begin(), s_Ops.end(), [](const Operator& a, const Operator& b) {
        if (a.side != b.side) return a.side < b.side;
        return a.name < b.name;
    });
}

const std::vector<Operator>& GetAll() {
    Init();
    return s_Ops;
}

std::vector<const Operator*> GetAttackers() {
    Init();
    std::vector<const Operator*> out;
    for (const auto& op : s_Ops)
        if (op.side == ATTACKER) out.push_back(&op);
    return out;
}

std::vector<const Operator*> GetDefenders() {
    Init();
    std::vector<const Operator*> out;
    for (const auto& op : s_Ops)
        if (op.side == DEFENDER) out.push_back(&op);
    return out;
}

const Operator* Find(const std::string& name) {
    Init();
    for (const auto& op : s_Ops)
        if (op.name == name) return &op;
    return nullptr;
}

} // namespace OperatorData
