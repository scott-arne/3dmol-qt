#pragma once

/// \file detail/checked_cast.h
/// \brief Internal: checked size_t -> uint32 cast for GPU buffer sizing.
///        Not public API.

#include <cstdint>
#include <limits>
#include <string>

#include "oe3dmol/geometry_errors.h"

namespace oe3dmol {
namespace detail {

/// \brief Cast `value` to uint32, throwing GeometryError if it exceeds UINT32_MAX.
/// QRhi buffer sizes, vertex/index counts, and instance counts are 32-bit; a
/// scene that overflows must fail with a clear message, not silently wrap.
/// \param value The size_t to narrow.
/// \param what  Context for the error message (e.g. "sphere vertex bytes").
inline std::uint32_t checked_u32(std::size_t value, const char* what) {
    if (value > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        throw GeometryError(std::string(what) +
                            ": value exceeds 32-bit limit required by QRhi");
    }
    return static_cast<std::uint32_t>(value);
}

}  // namespace detail
}  // namespace oe3dmol
