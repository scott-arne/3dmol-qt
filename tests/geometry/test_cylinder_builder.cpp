#include <gtest/gtest.h>

#include "oe3dmol/cylinder_builder.h"
#include "oe3dmol/cylinder_layouts.h"

namespace oe3dmol {
namespace test {

TEST(CylinderBuilderTest, BillboardHasFourVerticesSixIndices) {
    MeshBuffer m = build_cylinder_billboard();
    EXPECT_EQ(m.vertices.size(),
              static_cast<std::size_t>(4 * CylinderBillboardLayout::FLOATS_PER_VERTEX));
    EXPECT_EQ(m.indices.size(), 6u);
}

TEST(CylinderBuilderTest, BillboardVerticesCoverAllCornerCombinations) {
    MeshBuffer m = build_cylinder_billboard();
    constexpr int S = CylinderBillboardLayout::FLOATS_PER_VERTEX;
    // Collect the 4 (endpoint_sel, corner_sign) pairs; expect both p1 anchors
    // and both p2 anchors, each with both corner signs.
    bool p1_neg = false, p1_pos = false, p2_neg = false, p2_pos = false;
    for (int v = 0; v < 4; ++v) {
        const float sel = m.vertices[v * S + CylinderBillboardLayout::ENDPOINT_SEL_OFFSET];
        const float sign = m.vertices[v * S + CylinderBillboardLayout::CORNER_SIGN_OFFSET];
        if (sel > 0 && sign < 0) p1_neg = true;
        if (sel > 0 && sign > 0) p1_pos = true;
        if (sel < 0 && sign < 0) p2_neg = true;
        if (sel < 0 && sign > 0) p2_pos = true;
    }
    EXPECT_TRUE(p1_neg && p1_pos && p2_neg && p2_pos);
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
