#pragma once
#include <string>
#include <vector>

namespace OperatorData {

enum Side { ATTACKER, DEFENDER };

struct Operator {
    std::string name;
    Side        side;
    float       defaultRecoilX; // horizontal hint
    float       defaultRecoilY; // vertical hint
};

// Returns all operators (sorted alphabetically within each side)
const std::vector<Operator>& GetAll();

// Filtered views
std::vector<const Operator*> GetAttackers();
std::vector<const Operator*> GetDefenders();

// Lookup by name (case-sensitive), returns nullptr if not found
const Operator* Find(const std::string& name);

} // namespace OperatorData
