#pragma once

/// \file version.h
/// \brief Library version constants.

namespace oe3dmol {

constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;

/// \brief Return the library version as a "major.minor.patch" string.
const char* version_string();

}  // namespace oe3dmol
