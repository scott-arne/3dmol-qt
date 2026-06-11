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

/// \brief Base billboard vertex for the cylinder imposter: 2 floats.
///
/// endpoint_sel: +1 anchors the billboard at the bond start (p1), -1 at the
/// end (p2). corner_sign: +1 / -1 selects which lateral direction the vertex
/// shader pushes the corner by the bond radius. Four vertices (the 4 (sel,sign)
/// combinations) form the screen-facing quad the fragment shader ray-traces.
struct CylinderBillboardLayout {
    static constexpr int FLOATS_PER_VERTEX = 2;
    static constexpr int ENDPOINT_SEL_OFFSET = 0;  ///< floats (+1 = p1, -1 = p2)
    static constexpr int CORNER_SIGN_OFFSET = 1;    ///< floats (+1 / -1)
};

}  // namespace oe3dmol
