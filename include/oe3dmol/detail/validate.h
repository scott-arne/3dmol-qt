#pragma once

/// \file detail/validate.h
/// \brief Internal numeric-input validators for style setters. Not public API.

#include <cmath>
#include <string>

#include "oe3dmol/data_errors.h"

namespace oe3dmol {
namespace detail {

/// \brief Throw DataError unless `value` is finite and >= 0.
/// \param value The value to check.
/// \param what  Field name for the error message (e.g. "SphereStyle::radius").
inline double require_finite_nonnegative(double value, const char* what) {
    if (!std::isfinite(value) || value < 0.0) {
        throw DataError(std::string(what) +
                        ": value must be finite and non-negative");
    }
    return value;
}

}  // namespace detail
}  // namespace oe3dmol
