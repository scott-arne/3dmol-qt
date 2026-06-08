#include "oe3dmol/vdw_radii.h"

#include <cctype>
#include <unordered_map>

namespace oe3dmol {

namespace {

// Ported verbatim from 3dmol.js GLModel.ts vdwRadii.
const std::unordered_map<std::string, double>& vdw_table() {
    static const std::unordered_map<std::string, double> table = {
        {"H", 1.2},  {"He", 1.4},  {"Li", 1.82}, {"Be", 1.53}, {"B", 1.92},
        {"C", 1.7},  {"N", 1.55},  {"O", 1.52},  {"F", 1.47},  {"Ne", 1.54},
        {"Na", 2.27}, {"Mg", 1.73}, {"Al", 1.84}, {"Si", 2.1}, {"P", 1.8},
        {"S", 1.8},  {"Cl", 1.75}, {"Ar", 1.88}, {"K", 2.75},  {"Ca", 2.31},
        {"Ni", 1.63}, {"Cu", 1.4}, {"Zn", 1.39}, {"Ga", 1.87}, {"Ge", 2.11},
        {"As", 1.85}, {"Se", 1.9}, {"Br", 1.85}, {"Kr", 2.02}, {"Rb", 3.03},
        {"Sr", 2.49}, {"Pd", 1.63}, {"Ag", 1.72}, {"Cd", 1.58}, {"In", 1.93},
        {"Sn", 2.17}, {"Sb", 2.06}, {"Te", 2.06}, {"I", 1.98},  {"Xe", 2.16},
        {"Cs", 3.43}, {"Ba", 2.68}, {"Pt", 1.75}, {"Au", 1.66}, {"Hg", 1.55},
    };
    return table;
}

}  // namespace

double vdw_radius(const std::string& element) {
    const auto& table = vdw_table();
    auto it = table.find(element);
    if (it != table.end()) {
        return it->second;
    }
    if (element.size() > 1) {
        std::string normalized = element;
        normalized[0] = static_cast<char>(std::toupper(normalized[0]));
        for (std::size_t i = 1; i < normalized.size(); ++i) {
            normalized[i] = static_cast<char>(std::tolower(normalized[i]));
        }
        it = table.find(normalized);
        if (it != table.end()) {
            return it->second;
        }
    }
    return DEFAULT_SPHERE_RADIUS;
}

}  // namespace oe3dmol
