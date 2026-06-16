#include <gtest/gtest.h>

#include "oe3dmol/bounding_box.h"

namespace oe3dmol {
namespace test {

TEST(BoundingBoxTest, EmptyIsInvalid) {
    BoundingBox b;
    EXPECT_FALSE(b.valid());
}

TEST(BoundingBoxTest, SinglePointCenterIsThatPoint) {
    BoundingBox b;
    b.include(QVector3D(1.0f, 2.0f, 3.0f));
    EXPECT_TRUE(b.valid());
    EXPECT_FLOAT_EQ(b.center().x(), 1.0f);
    EXPECT_FLOAT_EQ(b.center().y(), 2.0f);
    EXPECT_FLOAT_EQ(b.center().z(), 3.0f);
}

TEST(BoundingBoxTest, RadiusExpandsBounds) {
    BoundingBox b;
    b.include(QVector3D(0.0f, 0.0f, 0.0f), 2.0f);
    EXPECT_FLOAT_EQ(b.min().x(), -2.0f);
    EXPECT_FLOAT_EQ(b.max().x(), 2.0f);
}

TEST(BoundingBoxTest, CenterIsMidpointOfTwoPoints) {
    BoundingBox b;
    b.include(QVector3D(-1.0f, 0.0f, 0.0f));
    b.include(QVector3D(3.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(b.center().x(), 1.0f);
}

TEST(BoundingBoxTest, BoundingRadiusCoversCorners) {
    BoundingBox b;
    b.include(QVector3D(-1.0f, -1.0f, -1.0f));
    b.include(QVector3D(1.0f, 1.0f, 1.0f));
    // half-diagonal = sqrt(3) ~ 1.7320508
    EXPECT_NEAR(b.bounding_radius(), 1.7320508f, 1e-5f);
}

TEST(BoundingBoxTest, MergeIncludesAnotherBox) {
    BoundingBox a;
    a.include(QVector3D(0, 0, 0), 1.0f);   // [-1,1]^3
    BoundingBox b;
    b.include(QVector3D(5, 0, 0), 1.0f);   // x in [4,6]
    a.include(b);
    EXPECT_TRUE(a.valid());
    EXPECT_NEAR(a.min().x(), -1.0f, 1e-4f);
    EXPECT_NEAR(a.max().x(), 6.0f, 1e-4f);
}

TEST(BoundingBoxTest, MergeWithInvalidBoxIsNoOp) {
    BoundingBox a;
    a.include(QVector3D(0, 0, 0), 1.0f);
    BoundingBox invalid;
    a.include(invalid);
    EXPECT_TRUE(a.valid());
    EXPECT_NEAR(a.max().x(), 1.0f, 1e-4f);
}

TEST(BoundingBoxTest, MergeIntoInvalidAdoptsOther) {
    BoundingBox a;  // invalid
    BoundingBox b;
    b.include(QVector3D(2, 2, 2), 0.5f);
    a.include(b);
    EXPECT_TRUE(a.valid());
    EXPECT_NEAR(a.min().x(), 1.5f, 1e-4f);
}

}  // namespace test
}  // namespace oe3dmol
