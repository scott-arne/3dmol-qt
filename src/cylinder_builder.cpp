#include "oe3dmol/cylinder_builder.h"

#include <cmath>

#include "oe3dmol/cylinder_layouts.h"
#include "oe3dmol/geometry_errors.h"

namespace oe3dmol {

namespace {

constexpr float PI_F = 3.14159265358979323846f;

void push_vertex(std::vector<float>& v, float px, float py, float pz,
                 float nx, float ny, float nz) {
    v.push_back(px);
    v.push_back(py);
    v.push_back(pz);
    v.push_back(nx);
    v.push_back(ny);
    v.push_back(nz);
}

}  // namespace

MeshBuffer build_unit_cylinder(int segments) {
    if (segments < 3) {
        throw GeometryError("build_unit_cylinder: segments must be >= 3");
    }

    MeshBuffer mesh;
    auto& vtx = mesh.vertices;
    auto& idx = mesh.indices;

    // --- Side surface: 2 rings (z=0, z=1) of `segments` vertices, radial normals.
    for (int ring = 0; ring < 2; ++ring) {
        const float z = static_cast<float>(ring);
        for (int s = 0; s < segments; ++s) {
            const float theta = (2.0f * PI_F * static_cast<float>(s)) /
                                static_cast<float>(segments);
            const float cx = std::cos(theta);
            const float cy = std::sin(theta);
            push_vertex(vtx, cx, cy, z, cx, cy, 0.0f);  // radial normal
        }
    }
    for (int s = 0; s < segments; ++s) {
        const std::uint32_t b0 = static_cast<std::uint32_t>(s);
        const std::uint32_t b1 = static_cast<std::uint32_t>((s + 1) % segments);
        const std::uint32_t t0 = b0 + static_cast<std::uint32_t>(segments);
        const std::uint32_t t1 = b1 + static_cast<std::uint32_t>(segments);
        idx.push_back(b0); idx.push_back(b1); idx.push_back(t1);
        idx.push_back(b0); idx.push_back(t1); idx.push_back(t0);
    }

    // --- Bottom cap: center + fan, normal -Z.
    const std::uint32_t bottom_center = static_cast<std::uint32_t>(
        vtx.size() / CylinderVertexLayout::FLOATS_PER_VERTEX);
    push_vertex(vtx, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f);
    const std::uint32_t bottom_rim_start = bottom_center + 1;
    for (int s = 0; s < segments; ++s) {
        const float theta = (2.0f * PI_F * static_cast<float>(s)) /
                            static_cast<float>(segments);
        push_vertex(vtx, std::cos(theta), std::sin(theta), 0.0f, 0.0f, 0.0f, -1.0f);
    }
    for (int s = 0; s < segments; ++s) {
        const std::uint32_t r0 = bottom_rim_start + static_cast<std::uint32_t>(s);
        const std::uint32_t r1 = bottom_rim_start +
            static_cast<std::uint32_t>((s + 1) % segments);
        idx.push_back(bottom_center); idx.push_back(r1); idx.push_back(r0);
    }

    // --- Top cap: center + fan, normal +Z.
    const std::uint32_t top_center = static_cast<std::uint32_t>(
        vtx.size() / CylinderVertexLayout::FLOATS_PER_VERTEX);
    push_vertex(vtx, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    const std::uint32_t top_rim_start = top_center + 1;
    for (int s = 0; s < segments; ++s) {
        const float theta = (2.0f * PI_F * static_cast<float>(s)) /
                            static_cast<float>(segments);
        push_vertex(vtx, std::cos(theta), std::sin(theta), 1.0f, 0.0f, 0.0f, 1.0f);
    }
    for (int s = 0; s < segments; ++s) {
        const std::uint32_t r0 = top_rim_start + static_cast<std::uint32_t>(s);
        const std::uint32_t r1 = top_rim_start +
            static_cast<std::uint32_t>((s + 1) % segments);
        idx.push_back(top_center); idx.push_back(r0); idx.push_back(r1);
    }

    return mesh;
}

std::vector<float> build_cylinder_instances(const BondData& bonds) {
    std::vector<float> inst;
    inst.reserve(bonds.size() * CylinderInstanceLayout::FLOATS_PER_INSTANCE);
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
