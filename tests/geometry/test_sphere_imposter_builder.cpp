#include <gtest/gtest.h>

#include "oe3dmol/sphere_imposter_builder.h"

namespace oe3dmol {
namespace test {

namespace {
AtomData one_atom() {
    AtomData d;
    d.add(QVector3D(1.0f, 2.0f, 3.0f), 1.7f, Color(255, 0, 0), 0u);
    return d;
}
}  // namespace

TEST(SphereImposterBuilderTest, FourVerticesPerAtom) {
    MeshBuffer m = build_sphere_imposters(one_atom());
    EXPECT_EQ(m.vertex_count(), 4u);
}

TEST(SphereImposterBuilderTest, SixIndicesPerAtom) {
    MeshBuffer m = build_sphere_imposters(one_atom());
    EXPECT_EQ(m.index_count(), 6u);
}

TEST(SphereImposterBuilderTest, AllVerticesAtAtomCenter) {
    MeshBuffer m = build_sphere_imposters(one_atom());
    constexpr int S = VertexLayout::FLOATS_PER_VERTEX;
    for (int v = 0; v < 4; ++v) {
        EXPECT_FLOAT_EQ(m.vertices[v * S + 0], 1.0f);
        EXPECT_FLOAT_EQ(m.vertices[v * S + 1], 2.0f);
        EXPECT_FLOAT_EQ(m.vertices[v * S + 2], 3.0f);
    }
}

TEST(SphereImposterBuilderTest, NormalsEncodeCornerOffsets) {
    MeshBuffer m = build_sphere_imposters(one_atom());
    constexpr int S = VertexLayout::FLOATS_PER_VERTEX;
    constexpr int N = VertexLayout::NORMAL_OFFSET;
    const float r = 1.7f;
    const float expected[4][3] = {
        {-r,  r, 0.0f},
        {-r, -r, 0.0f},
        { r, -r, 0.0f},
        { r,  r, 0.0f},
    };
    for (int v = 0; v < 4; ++v) {
        EXPECT_FLOAT_EQ(m.vertices[v * S + N + 0], expected[v][0]);
        EXPECT_FLOAT_EQ(m.vertices[v * S + N + 1], expected[v][1]);
        EXPECT_FLOAT_EQ(m.vertices[v * S + N + 2], expected[v][2]);
    }
}

TEST(SphereImposterBuilderTest, ColorsNormalizedToUnitFloat) {
    MeshBuffer m = build_sphere_imposters(one_atom());
    constexpr int S = VertexLayout::FLOATS_PER_VERTEX;
    constexpr int C = VertexLayout::COLOR_OFFSET;
    EXPECT_FLOAT_EQ(m.vertices[C + 0], 1.0f);  // red 255/255
    EXPECT_FLOAT_EQ(m.vertices[C + 1], 0.0f);
    EXPECT_FLOAT_EQ(m.vertices[C + 2], 0.0f);
    EXPECT_FLOAT_EQ(m.vertices[C + 3], 1.0f);  // opaque alpha
}

TEST(SphereImposterBuilderTest, AlphaCarriedPerVertex) {
    AtomData d;
    Color c(0, 255, 0);
    c.setAlphaF(0.5f);
    d.add(QVector3D(0, 0, 0), 1.0f, c, 0u);
    MeshBuffer m = build_sphere_imposters(d);
    constexpr int S = VertexLayout::FLOATS_PER_VERTEX;
    constexpr int C = VertexLayout::COLOR_OFFSET;
    for (int v = 0; v < 4; ++v) {
        // QColor stores alpha as 8-bit, so 0.5f round-trips to ~0.500008;
        // tolerate one quantization step (1/255).
        EXPECT_NEAR(m.vertices[v * S + C + 3], 0.5f, 1.0f / 255.0f);
    }
}

TEST(SphereImposterBuilderTest, FaceWindingMatches3dmol) {
    MeshBuffer m = build_sphere_imposters(one_atom());
    EXPECT_EQ(m.indices[0], 0u);
    EXPECT_EQ(m.indices[1], 1u);
    EXPECT_EQ(m.indices[2], 2u);
    EXPECT_EQ(m.indices[3], 2u);
    EXPECT_EQ(m.indices[4], 3u);
    EXPECT_EQ(m.indices[5], 0u);
}

TEST(SphereImposterBuilderTest, SecondAtomIndicesAreOffset) {
    AtomData d = one_atom();
    d.add(QVector3D(0.0f, 0.0f, 0.0f), 1.0f, Color(0, 255, 0), 1u);
    MeshBuffer m = build_sphere_imposters(d);
    EXPECT_EQ(m.vertex_count(), 8u);
    EXPECT_EQ(m.index_count(), 12u);
    EXPECT_EQ(m.indices[6], 4u);
    EXPECT_EQ(m.indices[7], 5u);
    EXPECT_EQ(m.indices[8], 6u);
}

}  // namespace test
}  // namespace oe3dmol
