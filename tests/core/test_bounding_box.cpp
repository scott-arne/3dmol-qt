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

}  // namespace test
}  // namespace oe3dmol
