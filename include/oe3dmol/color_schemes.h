#pragma once

/// \file color_schemes.h
/// \brief Element-to-color mappings ported from 3dmol.js colors.ts.

#include <string>

#include "oe3dmol/color.h"

namespace oe3dmol {

/// \brief Named element color schemes supported in v1.
enum class ColorScheme {
    Jmol,     ///< Jmol-like element colors (3dmol default).
    Default,  ///< 3dmol "defaultColors" (rasmol-based).
};

/// \brief Resolve an element symbol to a color under the given scheme.
///
/// Matches 3dmol.js semantics: lookup is by element symbol; unknown
/// elements fall back to the scheme's default color (0xff1493).
///
/// \param scheme Color scheme to use.
/// \param element Element symbol (e.g. "C", "Fe"); case-insensitive on
///        the first/second letter as in 3dmol.
/// \returns The element's color, or the default color if unmapped.
Color element_color(ColorScheme scheme, const std::string& element);

/// \brief The 3dmol default color for unmapped elements (0xff1493).
Color default_element_color();

}  // namespace oe3dmol
