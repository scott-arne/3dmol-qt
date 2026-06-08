#pragma once

/// \file style.h
/// \brief Sphere (VDW) representation style spec, mirroring 3dmol's SphereStyleSpec.

#include <optional>
#include <string>

#include "oe3dmol/color_schemes.h"

namespace oe3dmol {

/// \brief Style parameters for the sphere (VDW) representation.
///
/// Mirrors 3dmol.js SphereStyleSpec for the fields relevant to v1.
/// Fluent setters return *this for 3dmol-style chaining.
class SphereStyle {
public:
    /// \brief Set an explicit radius (overrides VDW lookup). \returns *this.
    SphereStyle& radius(double r) { radius_ = r; return *this; }
    /// \brief Set the VDW-radius scale factor. \returns *this.
    SphereStyle& scale(double s) { scale_ = s; return *this; }
    /// \brief Set the color scheme. \returns *this.
    SphereStyle& color(ColorScheme c) { color_ = c; return *this; }
    /// \brief Set opacity in [0,1]. \returns *this.
    SphereStyle& opacity(double o) { opacity_ = o; return *this; }

    const std::optional<double>& radius() const { return radius_; }
    const std::optional<double>& scale() const { return scale_; }
    ColorScheme color() const { return color_; }
    double opacity() const { return opacity_; }

private:
    std::optional<double> radius_;
    std::optional<double> scale_;
    ColorScheme color_ = ColorScheme::Jmol;
    double opacity_ = 1.0;
};

/// \brief Resolve the rendered radius for an element under a style.
///
/// Implements 3dmol's getRadiusFromStyle precedence:
///   explicit radius -> VDW(element) -> DEFAULT_SPHERE_RADIUS,
/// then multiplied by scale if present.
///
/// \param style The sphere style.
/// \param element Element symbol.
/// \returns Resolved radius in Angstrom.
double resolve_radius(const SphereStyle& style, const std::string& element);

}  // namespace oe3dmol
