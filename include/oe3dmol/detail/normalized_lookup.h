#pragma once

/// \file detail/normalized_lookup.h
/// \brief Internal: case-normalized element-symbol lookup shared by the color
///        and VDW-radius tables. Not part of the public API.

#include <cctype>
#include <optional>
#include <string>
#include <unordered_map>

namespace oe3dmol {
namespace detail {

/// \brief Look up `key` in `table`, first exact, then Title-cased (first byte
/// upper, rest lower) as a fallback for element symbols like "fe" -> "Fe".
///
/// Element symbols are ASCII, but the byte is cast to `unsigned char` before
/// `std::toupper`/`std::tolower` to avoid the undefined behavior those functions
/// have for negative `char` values.
///
/// \returns the mapped value if found by either spelling, else std::nullopt.
template <typename V>
std::optional<V> normalized_lookup(const std::unordered_map<std::string, V>& table,
                                   const std::string& key) {
    auto it = table.find(key);
    if (it != table.end()) {
        return it->second;
    }
    if (key.size() > 1) {
        std::string normalized = key;
        normalized[0] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(normalized[0])));
        for (std::size_t i = 1; i < normalized.size(); ++i) {
            normalized[i] = static_cast<char>(
                std::tolower(static_cast<unsigned char>(normalized[i])));
        }
        it = table.find(normalized);
        if (it != table.end()) {
            return it->second;
        }
    }
    return std::nullopt;
}

}  // namespace detail
}  // namespace oe3dmol
