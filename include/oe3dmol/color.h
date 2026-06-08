#pragma once

/// \file color.h
/// \brief RGB color type used throughout the renderer.

#include <QColor>

namespace oe3dmol {

/// \brief Alias for Qt's color type.
///
/// QColor is a headless-safe CPU value type. Using it directly lets us
/// reuse Qt's hex and CSS-named-color parsing, which maps onto 3dmol.js
/// color specifications.
using Color = QColor;

/// \brief Construct a Color from a 0xRRGGBB integer (matches 3dmol's ColorSpec).
///
/// \param rgb Packed 24-bit color, e.g. 0xff0d0d.
/// \returns Opaque QColor with the given red/green/blue channels.
inline Color color_from_hex(unsigned int rgb) {
    return QColor(static_cast<int>((rgb >> 16) & 0xff),
                  static_cast<int>((rgb >> 8) & 0xff),
                  static_cast<int>(rgb & 0xff));
}

}  // namespace oe3dmol
