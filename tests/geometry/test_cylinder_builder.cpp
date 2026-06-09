#include <gtest/gtest.h>

#include "oe3dmol/cylinder_builder.h"
#include "oe3dmol/cylinder_layouts.h"
#include "oe3dmol/geometry_errors.h"

#include <cmath>

namespace oe3dmol {
namespace test {

TEST(CylinderBuilderTest, RejectsTooFewSegments) {
    EXPECT_THROW(build_unit_cylinder(2), GeometryError);
}

TEST(CylinderBuilderTest, UnitCylinderHasExpectedVertexStride) {
    MeshBuffer m = build_unit_cylinder(16);
    EXPECT_EQ(m.vertices.size() % CylinderVertexLayout::FLOATS_PER_VERTEX, 0u);
    EXPECT_GT(m.vertices.size(), 0u);
    EXPECT_GT(m.indices.size(), 0u);
}

TEST(CylinderBuilderTest, SideVerticesLieWithinUnitRadiusAndHeight) {
    MeshBuffer m = build_unit_cylinder(16);
    constexpr int S = CylinderVertexLayout::FLOATS_PER_VERTEX;
    const std::size_t vcount = m.vertices.size() / S;
    for (std::size_t v = 0; v < vcount; ++v) {
        const float x = m.vertices[v * S + 0];
        const float y = m.vertices[v * S + 1];
        const float z = m.vertices[v * S + 2];
        EXPECT_LE(std::sqrt(x * x + y * y), 1.0f + 1e-4f);
        EXPECT_GE(z, -1e-4f);
        EXPECT_LE(z, 1.0f + 1e-4f);
    }
}

TEST(CylinderBuilderTest, SideNormalsAreUnitLength) {
    MeshBuffer m = build_unit_cylinder(16);
    constexpr int S = CylinderVertexLayout::FLOATS_PER_VERTEX;
    constexpr int N = CylinderVertexLayout::NORMAL_OFFSET;
    const std::size_t vcount = m.vertices.size() / S;
    for (std::size_t v = 0; v < vcount; ++v) {
        const float nx = m.vertices[v * S + N + 0];
        const float ny = m.vertices[v * S + N + 1];
        const float nz = m.vertices[v * S + N + 2];
        EXPECT_NEAR(std::sqrt(nx * nx + ny * ny + nz * nz), 1.0f, 1e-4f);
    }
}

TEST(CylinderBuilderTest, InstancesPackStartEndRadiusColor) {
    BondData b;
    b.add(QVector3D(0, 0, 0), QVector3D(1, 0, 0), 0.25f, Color(255, 0, 0), 0u);
    std::vector<float> inst = build_cylinder_instances(b);
    ASSERT_EQ(inst.size(),
              static_cast<std::size_t>(CylinderInstanceLayout::FLOATS_PER_INSTANCE));
    constexpr int SO = CylinderInstanceLayout::START_OFFSET;
    constexpr int EO = CylinderInstanceLayout::END_OFFSET;
    constexpr int RO = CylinderInstanceLayout::RADIUS_OFFSET;
    constexpr int CO = CylinderInstanceLayout::COLOR_OFFSET;
    EXPECT_FLOAT_EQ(inst[SO + 0], 0.0f);
    EXPECT_FLOAT_EQ(inst[EO + 0], 1.0f);
    EXPECT_FLOAT_EQ(inst[RO], 0.25f);
    EXPECT_FLOAT_EQ(inst[CO + 0], 1.0f);
    EXPECT_FLOAT_EQ(inst[CO + 1], 0.0f);
    EXPECT_FLOAT_EQ(inst[CO + 2], 0.0f);
    EXPECT_FLOAT_EQ(inst[CO + 3], 1.0f);
}

TEST(CylinderBuilderTest, EmptyBondsProduceEmptyInstances) {
    std::vector<float> inst = build_cylinder_instances(BondData{});
    EXPECT_TRUE(inst.empty());
}

}  // namespace test
}  // namespace oe3dmol
