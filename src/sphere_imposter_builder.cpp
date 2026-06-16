#include "oe3dmol/sphere_imposter_builder.h"

#include "oe3dmol/detail/checked_cast.h"

#include <cassert>

namespace oe3dmol {

MeshBuffer build_sphere_imposters(const AtomData& atoms) {
    MeshBuffer mesh;
    const std::size_t n = atoms.size();
    assert(atoms.radii.size() == n && atoms.colors.size() == n &&
           "AtomData SoA arrays must be the same length");
    constexpr int S = VertexLayout::FLOATS_PER_VERTEX;

    // QRhi index/vertex buffers are 32-bit; reject scenes that would overflow.
    (void)detail::checked_u32(n * 4, "sphere vertex count");
    (void)detail::checked_u32(n * 6, "sphere index count");

    mesh.vertices.resize(n * 4 * S);
    mesh.indices.resize(n * 6);

    for (std::size_t a = 0; a < n; ++a) {
        const QVector3D& c = atoms.positions[a];
        const float r = atoms.radii[a];
        const Color& col = atoms.colors[a];
        const float cr = static_cast<float>(col.redF());
        const float cg = static_cast<float>(col.greenF());
        const float cb = static_cast<float>(col.blueF());
        const float ca = static_cast<float>(col.alphaF());

        const float corner[4][2] = {
            {-r,  r},
            {-r, -r},
            { r, -r},
            { r,  r},
        };

        const std::size_t base_v = a * 4;
        for (int v = 0; v < 4; ++v) {
            float* p = &mesh.vertices[(base_v + v) * S];
            p[VertexLayout::POSITION_OFFSET + 0] = c.x();
            p[VertexLayout::POSITION_OFFSET + 1] = c.y();
            p[VertexLayout::POSITION_OFFSET + 2] = c.z();
            p[VertexLayout::NORMAL_OFFSET + 0] = corner[v][0];
            p[VertexLayout::NORMAL_OFFSET + 1] = corner[v][1];
            p[VertexLayout::NORMAL_OFFSET + 2] = 0.0f;
            p[VertexLayout::COLOR_OFFSET + 0] = cr;
            p[VertexLayout::COLOR_OFFSET + 1] = cg;
            p[VertexLayout::COLOR_OFFSET + 2] = cb;
            p[VertexLayout::COLOR_OFFSET + 3] = ca;
        }

        // base_v = a*4 < n*4, already validated to fit in uint32 by the
        // checked_u32(n*4, ...) guard above, so a plain cast is safe here.
        const std::uint32_t v0 = static_cast<std::uint32_t>(base_v);
        std::uint32_t* idx = &mesh.indices[a * 6];
        idx[0] = v0;
        idx[1] = v0 + 1;
        idx[2] = v0 + 2;
        idx[3] = v0 + 2;
        idx[4] = v0 + 3;
        idx[5] = v0;
    }

    return mesh;
}

}  // namespace oe3dmol
