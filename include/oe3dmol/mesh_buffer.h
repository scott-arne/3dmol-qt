#pragma once

/// \file mesh_buffer.h
/// \brief CPU-side interleaved vertex/index buffer (no GPU types).

#include <cstdint>
#include <vector>

namespace oe3dmol {

/// \brief Interleaved vertex layout for sphere imposters.
///
/// Per vertex: position(xyz) + normal(xyz, encodes corner offset) + color(rgba)
/// = 10 floats. Matches the attributes consumed by sphereimposter.vert. The
/// color alpha carries per-atom opacity.
struct VertexLayout {
    static constexpr int FLOATS_PER_VERTEX = 10;
    static constexpr int POSITION_OFFSET = 0;  ///< floats
    static constexpr int NORMAL_OFFSET = 3;    ///< floats
    static constexpr int COLOR_OFFSET = 6;     ///< floats (rgba, 4 floats)
};

/// \brief CPU vertex + index data ready for GPU upload.
struct MeshBuffer {
    std::vector<float> vertices;     ///< Interleaved, FLOATS_PER_VERTEX per vertex.
    std::vector<std::uint32_t> indices;  ///< Triangle indices.

    /// \brief Number of vertices, assuming the sphere VertexLayout (10 floats).
    ///
    /// \note Valid only for sphere-imposter meshes. Meshes built with a
    ///       different layout (e.g. the 6-float cylinder mesh) must compute
    ///       their vertex count using their own layout stride, not this method.
    std::size_t vertex_count() const {
        return vertices.size() / VertexLayout::FLOATS_PER_VERTEX;
    }
    /// \brief Number of indices.
    std::size_t index_count() const { return indices.size(); }
};

}  // namespace oe3dmol
