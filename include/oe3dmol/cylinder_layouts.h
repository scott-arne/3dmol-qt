#pragma once

/// \file cylinder_layouts.h
/// \brief Vertex/instance layouts for instanced cylinder (stick) rendering.

namespace oe3dmol {

/// \brief Unit-cylinder mesh vertex layout: position(xyz) + normal(xyz) = 6 floats.
struct CylinderVertexLayout {
    static constexpr int FLOATS_PER_VERTEX = 6;
    static constexpr int POSITION_OFFSET = 0;  ///< floats
    static constexpr int NORMAL_OFFSET = 3;    ///< floats
};

/// \brief Per-instance (per half-bond) layout:
/// start(xyz) + end(xyz) + radius(1) + color(rgba) = 11 floats.
struct CylinderInstanceLayout {
    static constexpr int FLOATS_PER_INSTANCE = 11;
    static constexpr int START_OFFSET = 0;   ///< floats
    static constexpr int END_OFFSET = 3;     ///< floats
    static constexpr int RADIUS_OFFSET = 6;  ///< floats
    static constexpr int COLOR_OFFSET = 7;   ///< floats (rgba, 4 floats)
};

}  // namespace oe3dmol
