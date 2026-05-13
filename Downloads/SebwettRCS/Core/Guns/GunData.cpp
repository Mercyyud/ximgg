#include "GunData.h"
#include <unordered_map>

namespace GunData {

// ── Gun database ──────────────────────────────────────────────────────────
// Recoil values are starting-point presets. Users can still fine-tune via sliders.
// Format: { "Gun Name", recoilX, recoilY }
// Primaries listed first, then secondaries. Verified against live game (2025/Y11).

static const std::vector<Gun> s_Empty;

static const std::unordered_map<std::string, std::vector<Gun>>& GetDB() {
    static const std::unordered_map<std::string, std::vector<Gun>> db = {

    // ── ATTACKERS ──────────────────────────────────────────────────────────

    { "Ace",         {{ "AK-12",           0.0f,  7.5f },  // primary
                      { "M1014",           0.0f,  0.0f },  // primary
                      { "P9",              0.0f,  2.0f }} },// secondary

    { "Amaru",       {{ "G8A1",            0.0f,  5.0f },  // primary
                      { "Supernova",       0.0f,  0.0f },  // primary
                      { "SMG-11",          0.0f,  6.5f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f },  // secondary
                      { "ITA12S",          0.0f,  0.0f }} },// secondary

    { "Ash",         {{ "R4-C",            0.0f,  7.5f },  // primary
                      { "G36C",            0.0f,  6.5f },  // primary
                      { "M45 Meusoc",      0.0f,  2.5f },  // secondary
                      { "5.7 USG",         0.0f,  1.5f }} },// secondary

    { "Blackbeard",  {{ "MK17 CQB",        0.0f,  6.0f },  // primary
                      { "SR-25",           0.0f,  5.5f },  // primary
                      { "D-50",            0.0f,  3.5f }} },// secondary

    { "Blitz",       {{ "P12",             0.0f,  2.0f }} },// secondary only (shield primary)

    { "Brava",       {{ "PARA-308",        0.0f,  6.5f },  // primary
                      { "CAMRS",           0.0f,  5.0f },  // primary
                      { "Super Shorty",    0.0f,  0.0f },  // secondary
                      { "PRB92",           0.0f,  2.0f }} },// secondary

    { "Buck",        {{ "C8-SFW",          0.0f,  7.0f },  // primary
                      { "CAMRS",           0.0f,  5.0f },  // primary
                      { "MK1 9mm",         0.0f,  2.5f },  // secondary
                      { "Super Shorty",    0.0f,  0.0f }} },// secondary

    { "Capitao",     {{ "PARA-308",        0.0f,  6.5f },  // primary
                      { "M249",            0.0f,  8.0f },  // primary
                      { "PRB92",           0.0f,  2.0f },  // secondary
                      { "Super Shorty",    0.0f,  0.0f }} },// secondary

    { "Deimos",      {{ "AK-74M",          0.0f,  6.5f },  // primary
                      { "M590A1",          0.0f,  0.0f },  // primary
                      { ".44 Vendetta",    0.0f,  4.0f }} },// secondary (unique scoped revolver)

    { "Dokkaebi",    {{ "MK 14 EBR",       0.0f,  5.0f },  // primary
                      { "BOSG.12.2",       0.0f,  0.0f },  // primary
                      { "SMG-12",          0.0f,  6.5f },  // secondary
                      { "C75 Auto",        0.0f,  5.0f }} },// secondary

    { "Finka",       {{ "SPEAR .308",      0.0f,  6.5f },  // primary
                      { "6P41",            0.0f,  9.0f },  // primary
                      { "SASG-12",         0.0f,  0.0f },  // primary
                      { "PMM",             0.0f,  2.5f },  // secondary
                      { "GSH-18",          0.0f,  2.0f }} },// secondary

    { "Flores",      {{ "AR33",            0.0f,  6.5f },  // primary
                      { "SR-25",           0.0f,  5.5f },  // primary
                      { "GSH-18",          0.0f,  2.0f }} },// secondary

    { "Fuze",        {{ "6P41",            0.0f,  9.0f },  // primary
                      { "AK-12",           0.0f,  7.5f },  // primary
                      { "PMM",             0.0f,  2.5f },  // secondary
                      { "GSH-18",          0.0f,  2.0f }} },// secondary

    { "Glaz",        {{ "OTs-03",          0.0f,  4.5f },  // primary
                      { "PMM",             0.0f,  2.5f },  // secondary
                      { "Bearing 9",       0.0f,  5.0f }} },// secondary

    { "Gridlock",    {{ "F90",             0.0f,  7.0f },  // primary
                      { "M249 SAW",        0.0f,  8.5f },  // primary
                      { "Super Shorty",    0.0f,  0.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Grim",        {{ "552 Commando",    0.0f,  7.0f },  // primary
                      { "SG-CQB",          0.0f,  0.0f },  // primary
                      { "P226 MK 25",      0.0f,  2.0f },  // secondary
                      { "Bailiff 410",     0.0f,  0.0f }} },// secondary

    { "Hibana",      {{ "TYPE-89",         0.0f,  7.0f },  // primary
                      { "Bearing 9",       0.0f,  5.0f },  // secondary
                      { "P226 MK 25",      0.0f,  2.0f },  // secondary
                      { "Super Shorty",    0.0f,  0.0f }} },// secondary

    { "Iana",        {{ "ARX200",          0.0f,  7.5f },  // primary
                      { "G36C",            0.0f,  6.5f },  // primary
                      { "MK1 9mm",         0.0f,  2.5f }} },// secondary

    { "IQ",          {{ "AUG A2",          0.0f,  6.0f },  // primary
                      { "552 Commando",    0.0f,  7.0f },  // primary
                      { "G8A1",            0.0f,  5.0f },  // primary
                      { "P12",             0.0f,  2.0f }} },// secondary

    { "Jackal",      {{ "C7E",             0.0f,  7.0f },  // primary
                      { "PDW9",            0.0f,  5.5f },  // primary
                      { "ITA12L",          0.0f,  0.0f },  // primary
                      { "USP40",           0.0f,  2.5f }} },// secondary

    { "Kali",        {{ "LV Lance",        0.0f,  0.0f },  // primary (underbarrel launcher gadget)
                      { "CSRX 300",        0.0f, 10.0f },  // primary
                      { "C75 Auto",        0.0f,  5.0f },  // secondary
                      { "P226 MK 25",      0.0f,  2.0f }} },// secondary

    { "Lion",        {{ "V308",            0.0f,  7.5f },  // primary
                      { "417",             0.0f,  5.5f },  // primary
                      { "SG-CQB",          0.0f,  0.0f },  // primary
                      { "GONNE-6",         0.0f,  0.0f },  // secondary
                      { "P9",              0.0f,  2.0f }} },// secondary

    { "Maverick",    {{ "AR-15.50",        0.0f,  6.0f },  // primary
                      { "M4",              0.0f,  7.0f },  // primary
                      { "1911 TACOPS",     0.0f,  2.5f }} },// secondary

    { "Montagne",    {{ "P9",              0.0f,  2.0f }} },// secondary only (Le Roc Shield is gadget)

    { "Nokk",        {{ "FMG-9",           0.0f,  6.0f },  // primary
                      { "SIX12 SD",        0.0f,  0.0f },  // primary
                      { "5.7 USG",         0.0f,  1.5f },  // secondary
                      { "D-50",            0.0f,  3.5f }} },// secondary

    { "Nomad",       {{ "AK-74M",          0.0f,  6.5f },  // primary
                      { "ARX200",          0.0f,  7.5f },  // primary
                      { "PRB92",           0.0f,  2.0f },  // secondary
                      { ".44 Mag Semi-Auto",0.0f, 3.0f }} },// secondary

    { "Osa",         {{ "556XI",           0.0f,  7.5f },  // primary
                      { "PDW9",            0.0f,  5.5f },  // primary
                      { "PMM",             0.0f,  2.5f }} },// secondary

    { "Ram",         {{ "R4-C",            0.0f,  7.5f },  // primary
                      { "LMG-E",           0.0f,  8.0f },  // primary
                      { "Super Shorty",    0.0f,  0.0f },  // secondary
                      { "MK1 9mm",         0.0f,  2.5f }} },// secondary

    { "Rauora",      {{ "417",             0.0f,  5.5f },  // primary (DMR)
                      { "M249",            0.0f,  8.5f },  // primary (LMG)
                      { "Reaper MK2",      0.0f,  5.0f }} },// secondary

    { "Sens",        {{ "POF-9",           0.0f,  7.0f },  // primary
                      { "417",             0.0f,  5.5f },  // primary
                      { "C75 Auto",        0.0f,  5.5f }} },// secondary

    { "Sledge",      {{ "L85A2",           0.0f,  6.5f },  // primary
                      { "M590A1",          0.0f,  0.0f },  // primary
                      { "SMG-11",          0.0f,  7.0f },  // secondary
                      { "P226 MK 25",      0.0f,  2.0f }} },// secondary

    { "Solid Snake", {{ "F2",              0.0f,  8.0f },  // primary (assault rifle)
                      { "PMR90A2",         0.0f,  5.5f },  // primary (marksman rifle)
                      { "TACIT .45",       0.0f,  2.5f }} },// secondary (suppressed pistol)

    { "Striker",     {{ "M4 Super 90",     0.0f,  0.0f },  // primary
                      { "ACS12",           0.0f,  0.0f },  // primary
                      { "SPSMG9",          0.0f,  5.0f },  // secondary
                      { "USP40",           0.0f,  2.5f }} },// secondary

    { "Thatcher",    {{ "AR33",            0.0f,  6.5f },  // primary
                      { "L85A2",           0.0f,  6.5f },  // primary
                      { "M590A1",          0.0f,  0.0f },  // primary
                      { "P226 MK 25",      0.0f,  2.0f }} },// secondary

    { "Thermite",    {{ "556XI",           0.0f,  7.5f },  // primary
                      { "M1014",           0.0f,  0.0f },  // primary
                      { "M45 Meusoc",      0.0f,  2.5f },  // secondary
                      { "Super Shorty",    0.0f,  0.0f }} },// secondary

    { "Twitch",      {{ "F2",              0.0f,  8.5f },  // primary
                      { "417",             0.0f,  5.5f },  // primary
                      { "SG-CQB",          0.0f,  0.0f },  // primary
                      { "LFP586",          0.0f,  3.5f },  // secondary
                      { "P9",              0.0f,  2.0f }} },// secondary

    { "Ying",        {{ "T-95 LSW",        0.0f,  7.5f },  // primary
                      { "LFP586",          0.0f,  3.5f },  // secondary
                      { "Q-929",           0.0f,  2.0f }} },// secondary

    { "Zero",        {{ "SC3000K",         0.0f,  7.0f },  // primary
                      { "MP7",             0.0f,  5.5f },  // primary
                      { "5.7 USG",         0.0f,  1.5f },  // secondary
                      { "RG15",            0.0f,  2.0f }} },// secondary

    { "Zofia",       {{ "LMG-E",           0.0f,  8.0f },  // primary
                      { "M762",            0.0f,  7.5f },  // primary
                      { "RG15",            0.0f,  2.0f }} },// secondary

    // ── DEFENDERS ─────────────────────────────────────────────────────────

    { "Alibi",       {{ "Mx4 Storm",       0.0f,  5.0f },  // primary
                      { "ACS12",           0.0f,  0.0f },  // primary
                      { "Keratos .357",    0.0f,  3.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Aruni",       {{ "P10 RONI",        0.0f,  5.5f },  // primary
                      { "Mk 14 EBR",       0.0f,  5.0f },  // primary
                      { "PRB92",           0.0f,  2.0f },  // secondary
                      { "LFP586",          0.0f,  3.5f }} },// secondary

    { "Azami",       {{ "9x19VSN",         0.0f,  5.0f },  // primary
                      { "ACS12",           0.0f,  0.0f },  // primary
                      { "D-50",            0.0f,  3.5f }} },// secondary

    { "Bandit",      {{ "MP7",             0.0f,  5.5f },  // primary
                      { "M870",            0.0f,  0.0f },  // primary
                      { "Keratos .357",    0.0f,  3.0f },  // secondary (added Y10)
                      { "P12",             0.0f,  2.0f }} },// secondary

    { "Castle",      {{ "UMP45",           0.0f,  4.5f },  // primary
                      { "M870",            0.0f,  0.0f },  // primary
                      { "Super Shorty",    0.0f,  0.0f },  // secondary
                      { "5.7 USG",         0.0f,  1.5f },  // secondary
                      { "M45 Meusoc",      0.0f,  2.5f }} },// secondary

    { "Caveira",     {{ "M12",             0.0f,  5.0f },  // primary
                      { "SPAS-15",         0.0f,  0.0f },  // primary
                      { "Luison",          0.0f,  2.0f }} },// secondary (unique suppressed PRB92)

    { "Clash",       {{ "P-10C",           0.0f,  2.0f },  // secondary
                      { "SPSMG9",          0.0f,  5.0f }} },// secondary (CCE Shield is gadget)

    { "Doc",         {{ "MP5",             0.0f,  6.0f },  // primary
                      { "P90",             0.0f,  6.5f },  // primary
                      { "SG-CQB",          0.0f,  0.0f },  // primary
                      { "P9",              0.0f,  2.0f },  // secondary
                      { "LFP586",          0.0f,  3.5f },  // secondary
                      { "Bailiff 410",     0.0f,  0.0f }} },// secondary

    { "Echo",        {{ "MP5SD",           0.0f,  5.0f },  // primary
                      { "Supernova",       0.0f,  0.0f },  // primary
                      { "Bearing 9",       0.0f,  5.0f },  // secondary
                      { "P229",            0.0f,  2.0f }} },// secondary

    { "Ela",         {{ "Scorpion EVO 3 A1",0.0f, 7.0f },  // primary
                      { "FO-12",           0.0f,  0.0f },  // primary
                      { "RG15",            0.0f,  2.0f }} },// secondary

    { "Fenrir",      {{ "MP7",             0.0f,  5.5f },  // primary
                      { "SASG-12",         0.0f,  0.0f },  // primary
                      { "Bailiff 410",     0.0f,  0.0f },  // secondary
                      { "D-50",            0.0f,  3.5f }} },// secondary

    { "Frost",       {{ "Super 90",        0.0f,  0.0f },  // primary
                      { "9mm C1",          0.0f,  4.5f },  // primary
                      { "MK1 9mm",         0.0f,  2.5f },  // secondary
                      { "ITA12S",          0.0f,  0.0f }} },// secondary

    { "Goyo",        {{ "Vector .45 ACP",  0.0f,  6.0f },  // primary
                      { "TCSG12",          0.0f,  5.0f },  // primary
                      { "Keratos .357",    0.0f,  3.0f }} },// secondary

    { "Jager",       {{ "416-C Carbine",   0.0f,  6.5f },  // primary
                      { "M870",            0.0f,  0.0f },  // primary
                      { "P10C",            0.0f,  2.0f },  // secondary (added Y10)
                      { "P12",             0.0f,  2.0f }} },// secondary

    { "Kaid",        {{ "TCSG12",          0.0f,  5.0f },  // primary
                      { "AUG A3",          0.0f,  6.0f },  // primary
                      { "Keratos .357",    0.0f,  3.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Kapkan",      {{ "9x19VSN",         0.0f,  5.0f },  // primary
                      { "SASG-12",         0.0f,  0.0f },  // primary
                      { "PMM",             0.0f,  2.5f }} },// secondary

    { "Lesion",      {{ "T-5 SMG",         0.0f,  5.5f },  // primary
                      { "SIX12 SD",        0.0f,  0.0f },  // primary
                      { "Q-929",           0.0f,  2.0f },  // secondary
                      { "Super Shorty",    0.0f,  0.0f }} },// secondary

    { "Maestro",     {{ "ALDA 5.56",       0.0f,  6.5f },  // primary
                      { "ACS12",           0.0f,  0.0f },  // primary
                      { "Keratos .357",    0.0f,  3.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Melusi",      {{ "MP5",             0.0f,  5.5f },  // primary
                      { "Super 90",        0.0f,  0.0f },  // primary
                      { "RG15",            0.0f,  2.0f }} },// secondary

    { "Mira",        {{ "Vector .45 ACP",  0.0f,  6.0f },  // primary
                      { "ITA12L",          0.0f,  0.0f },  // primary
                      { "USP40",           0.0f,  2.5f },  // secondary
                      { "ITA12S",          0.0f,  0.0f }} },// secondary

    { "Mozzie",      {{ "Commando 9",      0.0f,  5.5f },  // primary
                      { "P10 RONI",        0.0f,  5.5f },  // primary
                      { "SDP 9mm",         0.0f,  2.0f }} },// secondary

    { "Mute",        {{ "MP5K",            0.0f,  5.5f },  // primary
                      { "M590A1",          0.0f,  0.0f },  // primary
                      { "SMG-11",          0.0f,  7.0f },  // secondary
                      { "P226 MK 25",      0.0f,  2.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Oryx",        {{ "T-5 SMG",         0.0f,  5.5f },  // primary
                      { "SPAS-12",         0.0f,  0.0f },  // primary
                      { "USP40",           0.0f,  2.5f }} },// secondary

    { "Pulse",       {{ "M1014",           0.0f,  0.0f },  // primary
                      { "UMP45",           0.0f,  4.5f },  // primary
                      { "M45 Meusoc",      0.0f,  2.5f }} },// secondary

    { "Rook",        {{ "MP5",             0.0f,  5.5f },  // primary
                      { "P90",             0.0f,  6.0f },  // primary
                      { "SG-CQB",          0.0f,  0.0f },  // primary
                      { "P9",              0.0f,  2.0f },  // secondary
                      { "LFP586",          0.0f,  3.5f },  // secondary
                      { "Bailiff 410",     0.0f,  0.0f }} },// secondary

    { "Sentry",      {{ "MP5K",            0.0f,  5.5f },  // primary
                      { "M870",            0.0f,  0.0f },  // primary
                      { "Super Shorty",    0.0f,  0.0f },  // secondary
                      { "P226 MK 25",      0.0f,  2.0f }} },// secondary

    { "Skopos",      {{ "PCX-33",          0.0f,  5.5f },  // primary (unique .308 AR)
                      { "P229",            0.0f,  2.0f }} },// secondary

    { "Smoke",       {{ "FMG-9",           0.0f,  5.5f },  // primary
                      { "M590A1",          0.0f,  0.0f },  // primary
                      { "SMG-11",          0.0f,  7.0f },  // secondary
                      { "P226 MK 25",      0.0f,  2.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Solis",       {{ "P90",             0.0f,  6.0f },  // primary
                      { "ITA12L",          0.0f,  0.0f },  // primary
                      { "SMG-11",          0.0f,  7.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Tachanka",    {{ "DP27",            0.0f,  5.5f },  // primary
                      { "9x19VSN",         0.0f,  5.0f },  // primary
                      { "Bearing 9",       0.0f,  5.0f },  // secondary
                      { "PMM",             0.0f,  2.5f }} },// secondary

    { "Thorn",       {{ "UZK50GI",         0.0f,  5.5f },  // primary
                      { "M870",            0.0f,  0.0f },  // primary
                      { "C75 Auto",        0.0f,  5.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Thunderbird", {{ "SPEAR .308",      0.0f,  7.0f },  // primary
                      { "SPAS-15",         0.0f,  0.0f },  // primary
                      { "Bearing 9",       0.0f,  5.0f },  // secondary
                      { "Q-929",           0.0f,  2.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Tubarao",     {{ "MPX",             0.0f,  5.5f },  // primary
                      { "TCSG12",          0.0f,  5.0f },  // primary
                      { "P226 MK 25",      0.0f,  2.0f }} },// secondary

    { "Valkyrie",    {{ "MPX",             0.0f,  5.5f },  // primary
                      { "SPAS-12",         0.0f,  0.0f },  // primary
                      { "D-50",            0.0f,  3.5f }} },// secondary

    { "Vigil",       {{ "K1A",             0.0f,  6.0f },  // primary
                      { "BOSG.12.2",       0.0f,  0.0f },  // primary
                      { "SMG-12",          0.0f,  6.5f },  // secondary
                      { "C75 Auto",        0.0f,  5.0f }} },// secondary

    { "Wamai",       {{ "AUG A2",          0.0f,  6.0f },  // primary
                      { "MP5K",            0.0f,  5.5f },  // primary
                      { "Keratos .357",    0.0f,  3.0f },  // secondary
                      { "GONNE-6",         0.0f,  0.0f }} },// secondary

    { "Warden",      {{ "M590A1",          0.0f,  0.0f },  // primary
                      { "MPX",             0.0f,  5.5f },  // primary
                      { "P-10C",           0.0f,  2.0f },  // secondary
                      { "SMG-12",          0.0f,  6.5f }}},  // secondary

    }; // end db
    return db;
}

const std::vector<Gun>& GetGunsForOperator(const std::string& operatorName) {
    auto& db = GetDB();
    auto it = db.find(operatorName);
    if (it != db.end()) return it->second;
    return s_Empty;
}

const Gun* FindGun(const std::string& operatorName, const std::string& gunName) {
    for (const auto& g : GetGunsForOperator(operatorName))
        if (g.name == gunName) return &g;
    return nullptr;
}

} // namespace GunData
