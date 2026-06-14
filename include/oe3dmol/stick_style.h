#pragma once

/// \file stick_style.h
/// \brief Stick (bond cylinder) representation style, mirroring 3dmol's StickStyleSpec.

#include "oe3dmol/color_schemes.h"

namespace oe3dmol {

/// \brief Default bond cylinder radius in Angstrom (matches 3dmol's stick default).
constexpr double STICK_DEFAULT_RADIUS = 0.25;

/// \brief How aromatic bonds are drawn.
///
/// Kekule: render the molecule's alternating single/double bond orders (no
/// special aromatic geometry) -- a clean classical look. Delocalized: render
/// aromatic ring bonds as normal full-radius cylinders plus a thin dashed circle
/// inside each aromatic ring (the "circle in the ring" depiction).
enum class AromaticStyle { Kekule, Delocalized };

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
    /// \brief Set how aromatic bonds are drawn. \returns *this. Default Kekule.
    StickStyle& aromatic_style(AromaticStyle a) { aromatic_style_ = a; return *this; }

    double radius() const { return radius_; }
    ColorScheme color() const { return color_; }
    double opacity() const { return opacity_; }
    bool single_bonds() const { return single_bonds_; }
    AromaticStyle aromatic_style() const { return aromatic_style_; }

private:
    double radius_ = STICK_DEFAULT_RADIUS;
    ColorScheme color_ = ColorScheme::Jmol;
    double opacity_ = 1.0;
    bool single_bonds_ = false;  // false => render bond order + aromaticity
    AromaticStyle aromatic_style_ = AromaticStyle::Kekule;
};

}  // namespace oe3dmol
