#pragma once

/// \file stick_style.h
/// \brief Stick (bond cylinder) representation style, mirroring 3dmol's StickStyleSpec.

#include "oe3dmol/color_schemes.h"

namespace oe3dmol {

/// \brief Default bond cylinder radius in Angstrom (matches 3dmol's stick default).
constexpr double STICK_DEFAULT_RADIUS = 0.25;

/// \brief Style parameters for the stick (bond) representation.
///
/// Fluent setters return *this for 3dmol-style chaining.
class StickStyle {
public:
    /// \brief Set the cylinder radius (Angstrom). \returns *this.
    StickStyle& radius(double r) { radius_ = r; return *this; }
    /// \brief Set the color scheme. \returns *this.
    StickStyle& color(ColorScheme c) { color_ = c; return *this; }
    /// \brief Set opacity in [0,1]. \returns *this.
    StickStyle& opacity(double o) { opacity_ = o; return *this; }
    /// \brief Draw every bond as a single cylinder, ignoring order/aromaticity.
    /// \returns *this. Default false (multiplicity and aromaticity are shown).
    StickStyle& single_bonds(bool s) { single_bonds_ = s; return *this; }

    double radius() const { return radius_; }
    ColorScheme color() const { return color_; }
    double opacity() const { return opacity_; }
    bool single_bonds() const { return single_bonds_; }

private:
    double radius_ = STICK_DEFAULT_RADIUS;
    ColorScheme color_ = ColorScheme::Jmol;
    double opacity_ = 1.0;
    bool single_bonds_ = false;  // false => render bond order + aromaticity
};

}  // namespace oe3dmol
