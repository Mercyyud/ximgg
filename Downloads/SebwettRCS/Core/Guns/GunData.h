#pragma once
#include <string>
#include <vector>

namespace GunData {

struct Gun {
    std::string name;
    float       defaultRecoilX; // horizontal compensation hint
    float       defaultRecoilY; // vertical compensation hint
};

// Returns all guns available to a given operator name.
// If the operator is unknown, returns an empty vector.
const std::vector<Gun>& GetGunsForOperator(const std::string& operatorName);

// Lookup a specific gun by name within an operator's pool. nullptr if not found.
const Gun* FindGun(const std::string& operatorName, const std::string& gunName);

} // namespace GunData
