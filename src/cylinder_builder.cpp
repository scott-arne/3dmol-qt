#include "oe3dmol/cylinder_builder.h"

#include "oe3dmol/cylinder_layouts.h"

#include <cassert>

namespace oe3dmol {

MeshBuffer build_cylinder_billboard() {
    MeshBuffer mesh;
    // 4 vertices: (endpoint_sel, corner_sign) for the 4 quad corners.
    // Two anchored at p1 (+1) with corner signs -/+, two at p2 (-1) with -/+.
    const float verts[4][2] = {
        {+1.0f, -1.0f},
        {+1.0f, +1.0f},
        {-1.0f, -1.0f},
        {-1.0f, +1.0f},
    };
    mesh.vertices.reserve(4 * CylinderBillboardLayout::FLOATS_PER_VERTEX);
    for (const auto& v : verts) {
        mesh.vertices.push_back(v[0]);
        mesh.vertices.push_back(v[1]);
    }
    // Two triangles: (0,1,2),(2,3,0) -- matches 3dmol's stick face winding.
    mesh.indices = {0u, 1u, 2u, 2u, 3u, 0u};
    return mesh;
}

std::vector<float> build_cylinder_instances(const BondData& bonds) {
    std::vector<float> inst;
    inst.reserve(bonds.size() * CylinderInstanceLayout::FLOATS_PER_INSTANCE);
    assert(bonds.ends.size() == bonds.size() && bonds.radii.size() == bonds.size() &&
           bonds.colors.size() == bonds.size() &&
           "BondData SoA arrays must be the same length");
    for (std::size_t i = 0; i < bonds.size(); ++i) {
        const QVector3D& s = bonds.starts[i];
        const QVector3D& e = bonds.ends[i];
        const Color& c = bonds.colors[i];
        inst.push_back(s.x()); inst.push_back(s.y()); inst.push_back(s.z());
        inst.push_back(e.x()); inst.push_back(e.y()); inst.push_back(e.z());
        inst.push_back(bonds.radii[i]);
        inst.push_back(static_cast<float>(c.redF()));
        inst.push_back(static_cast<float>(c.greenF()));
        inst.push_back(static_cast<float>(c.blueF()));
        inst.push_back(static_cast<float>(c.alphaF()));
    }
    return inst;
}

}  // namespace oe3dmol
