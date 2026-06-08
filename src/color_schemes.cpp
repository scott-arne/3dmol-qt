#include "oe3dmol/color_schemes.h"

#include <cctype>
#include <unordered_map>

namespace oe3dmol {

namespace {

constexpr unsigned int DEFAULT_COLOR = 0xff1493;

// Ported verbatim from 3dmol.js colors.ts elementColors.Jmol.
const std::unordered_map<std::string, unsigned int>& jmol_table() {
    static const std::unordered_map<std::string, unsigned int> table = {
        {"H", 0xffffff},
        {"He", 0xd9ffff}, {"HE", 0xd9ffff},
        {"Li", 0xcc80ff}, {"LI", 0xcc80ff},
        {"Be", 0xc2ff00}, {"BE", 0xc2ff00},
        {"B", 0xffb5b5},
        {"C", 0x909090},
        {"N", 0x3050f8},
        {"O", 0xff0d0d},
        {"F", 0x90e050},
        {"Ne", 0xb3e3f5}, {"NE", 0xb3e3f5},
        {"Na", 0xab5cf2}, {"NA", 0xab5cf2},
        {"Mg", 0x8aff00}, {"MG", 0x8aff00},
        {"Al", 0xbfa6a6}, {"AL", 0xbfa6a6},
        {"Si", 0xf0c8a0}, {"SI", 0xf0c8a0},
        {"P", 0xff8000},
        {"S", 0xffff30},
        {"Cl", 0x1ff01f}, {"CL", 0x1ff01f},
        {"Ar", 0x80d1e3}, {"AR", 0x80d1e3},
        {"K", 0x8f40d4},
        {"Ca", 0x3dff00}, {"CA", 0x3dff00},
        {"Sc", 0xe6e6e6}, {"SC", 0xe6e6e6},
        {"Ti", 0xbfc2c7}, {"TI", 0xbfc2c7},
        {"V", 0xa6a6ab},
        {"Cr", 0x8a99c7}, {"CR", 0x8a99c7},
        {"Mn", 0x9c7ac7}, {"MN", 0x9c7ac7},
        {"Fe", 0xe06633}, {"FE", 0xe06633},
        {"Co", 0xf090a0}, {"CO", 0xf090a0},
        {"Ni", 0x50d050}, {"NI", 0x50d050},
        {"Cu", 0xc88033}, {"CU", 0xc88033},
        {"Zn", 0x7d80b0}, {"ZN", 0x7d80b0},
        {"Ga", 0xc28f8f}, {"GA", 0xc28f8f},
        {"Ge", 0x668f8f}, {"GE", 0x668f8f},
        {"As", 0xbd80e3}, {"AS", 0xbd80e3},
        {"Se", 0xffa100}, {"SE", 0xffa100},
        {"Br", 0xa62929}, {"BR", 0xa62929},
        {"Kr", 0x5cb8d1}, {"KR", 0x5cb8d1},
        {"Rb", 0x702eb0}, {"RB", 0x702eb0},
        {"Sr", 0x00ff00}, {"SR", 0x00ff00},
        {"Y", 0x94ffff},
        {"Zr", 0x94e0e0}, {"ZR", 0x94e0e0},
        {"Nb", 0x73c2c9}, {"NB", 0x73c2c9},
        {"Mo", 0x54b5b5}, {"MO", 0x54b5b5},
        {"Tc", 0x3b9e9e}, {"TC", 0x3b9e9e},
        {"Ru", 0x248f8f}, {"RU", 0x248f8f},
        {"Rh", 0x0a7d8c}, {"RH", 0x0a7d8c},
        {"Pd", 0x006985}, {"PD", 0x006985},
        {"Ag", 0xc0c0c0}, {"AG", 0xc0c0c0},
        {"Cd", 0xffd98f}, {"CD", 0xffd98f},
        {"In", 0xa67573}, {"IN", 0xa67573},
        {"Sn", 0x668080}, {"SN", 0x668080},
        {"Sb", 0x9e63b5}, {"SB", 0x9e63b5},
        {"Te", 0xd47a00}, {"TE", 0xd47a00},
        {"I", 0x940094},
        {"Xe", 0x429eb0}, {"XE", 0x429eb0},
        {"Cs", 0x57178f}, {"CS", 0x57178f},
        {"Ba", 0x00c900}, {"BA", 0x00c900},
        {"La", 0x70d4ff}, {"LA", 0x70d4ff},
        {"Ce", 0xffffc7}, {"CE", 0xffffc7},
        {"Pr", 0xd9ffc7}, {"PR", 0xd9ffc7},
        {"Nd", 0xc7ffc7}, {"ND", 0xc7ffc7},
        {"Pm", 0xa3ffc7}, {"PM", 0xa3ffc7},
        {"Sm", 0x8fffc7}, {"SM", 0x8fffc7},
        {"Eu", 0x61ffc7}, {"EU", 0x61ffc7},
        {"Gd", 0x45ffc7}, {"GD", 0x45ffc7},
        {"Tb", 0x30ffc7}, {"TB", 0x30ffc7},
        {"Dy", 0x1fffc7}, {"DY", 0x1fffc7},
        {"Ho", 0x00ff9c}, {"HO", 0x00ff9c},
        {"Er", 0x00e675}, {"ER", 0x00e675},
        {"Tm", 0x00d452}, {"TM", 0x00d452},
        {"Yb", 0x00bf38}, {"YB", 0x00bf38},
        {"Lu", 0x00ab24}, {"LU", 0x00ab24},
        {"Hf", 0x4dc2ff}, {"HF", 0x4dc2ff},
        {"Ta", 0x4da6ff}, {"TA", 0x4da6ff},
        {"W", 0x2194d6},
        {"Re", 0x267dab}, {"RE", 0x267dab},
        {"Os", 0x266696}, {"OS", 0x266696},
        {"Ir", 0x175487}, {"IR", 0x175487},
        {"Pt", 0xd0d0e0}, {"PT", 0xd0d0e0},
        {"Au", 0xffd123}, {"AU", 0xffd123},
        {"Hg", 0xb8b8d0}, {"HG", 0xb8b8d0},
        {"Tl", 0xa6544d}, {"TL", 0xa6544d},
        {"Pb", 0x575961}, {"PB", 0x575961},
        {"Bi", 0x9e4fb5}, {"BI", 0x9e4fb5},
        {"Po", 0xab5c00}, {"PO", 0xab5c00},
        {"At", 0x754f45}, {"AT", 0x754f45},
        {"Rn", 0x428296}, {"RN", 0x428296},
        {"Fr", 0x420066}, {"FR", 0x420066},
        {"Ra", 0x007d00}, {"RA", 0x007d00},
        {"Ac", 0x70abfa}, {"AC", 0x70abfa},
        {"Th", 0x00baff}, {"TH", 0x00baff},
        {"Pa", 0x00a1ff}, {"PA", 0x00a1ff},
        {"U", 0x008fff},
        {"Np", 0x0080ff}, {"NP", 0x0080ff},
        {"Pu", 0x006bff}, {"PU", 0x006bff},
        {"Am", 0x545cf2}, {"AM", 0x545cf2},
        {"Cm", 0x785ce3}, {"CM", 0x785ce3},
        {"Bk", 0x8a4fe3}, {"BK", 0x8a4fe3},
        {"Cf", 0xa136d4}, {"CF", 0xa136d4},
        {"Es", 0xb31fd4}, {"ES", 0xb31fd4},
        {"Fm", 0xb31fba}, {"FM", 0xb31fba},
        {"Md", 0xb30da6}, {"MD", 0xb30da6},
        {"No", 0xbd0d87}, {"NO", 0xbd0d87},
        {"Lr", 0xc70066}, {"LR", 0xc70066},
        {"Rf", 0xcc0059}, {"RF", 0xcc0059},
        {"Db", 0xd1004f}, {"DB", 0xd1004f},
        {"Sg", 0xd90045}, {"SG", 0xd90045},
        {"Bh", 0xe00038}, {"BH", 0xe00038},
        {"Hs", 0xe6002e}, {"HS", 0xe6002e},
        {"Mt", 0xeb0026}, {"MT", 0xeb0026},
    };
    return table;
}

// 3dmol "defaultColors"/rasmol element colors (subset used in v1).
const std::unordered_map<std::string, unsigned int>& rasmol_table() {
    static const std::unordered_map<std::string, unsigned int> table = {
        {"H", 0xffffff},
        {"He", 0xffc0cb}, {"HE", 0xffc0cb},
        {"Li", 0xb22222}, {"LI", 0xb22222},
        {"B", 0x00ff00},
        {"C", 0xc8c8c8},
        {"N", 0x8f8fff},
        {"O", 0xf00000},
        {"F", 0xdaa520},
        {"Na", 0x0000ff}, {"NA", 0x0000ff},
        {"Mg", 0x228b22}, {"MG", 0x228b22},
        {"Al", 0x808090}, {"AL", 0x808090},
        {"Si", 0xdaa520}, {"SI", 0xdaa520},
        {"P", 0xffa500},
        {"S", 0xffc832},
        {"Cl", 0x00ff00}, {"CL", 0x00ff00},
        {"Ca", 0x808090}, {"CA", 0x808090},
        {"Fe", 0xffa500}, {"FE", 0xffa500},
        {"Br", 0xa52a2a}, {"BR", 0xa52a2a},
        {"I", 0xa020f0},
        {"Au", 0xdaa520}, {"AU", 0xdaa520},
    };
    return table;
}

unsigned int lookup(const std::unordered_map<std::string, unsigned int>& table,
                    const std::string& element) {
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
    return DEFAULT_COLOR;
}

}  // namespace

Color element_color(ColorScheme scheme, const std::string& element) {
    const auto& table = (scheme == ColorScheme::Jmol) ? jmol_table() : rasmol_table();
    return color_from_hex(lookup(table, element));
}

Color default_element_color() {
    return color_from_hex(DEFAULT_COLOR);
}

}  // namespace oe3dmol
