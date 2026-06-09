#include <gtest/gtest.h>

#include "oe3dmol/arcball.h"

#include <cmath>

namespace oe3dmol {
namespace test {

TEST(ArcballTest, ZeroDragIsIdentity) {
    QQuaternion q = arcball_rotation(QPointF(100, 100), QPointF(100, 100), QSize(200, 200));
    EXPECT_NEAR(q.scalar(), 1.0f, 1e-4f);
    EXPECT_NEAR(q.vector().length(), 0.0f, 1e-4f);
}

TEST(ArcballTest, HorizontalDragRotatesAboutYAxis) {
    QQuaternion q = arcball_rotation(QPointF(80, 100), QPointF(120, 100), QSize(200, 200));
    QVector3D axis;
    float angle = 0.0f;
    q.getAxisAndAngle(&axis, &angle);
    EXPECT_GT(angle, 1.0f);
    EXPECT_GT(std::abs(axis.y()), std::abs(axis.x()));
    EXPECT_GT(std::abs(axis.y()), std::abs(axis.z()));
}

TEST(ArcballTest, VerticalDragRotatesAboutXAxis) {
    QQuaternion q = arcball_rotation(QPointF(100, 80), QPointF(100, 120), QSize(200, 200));
    QVector3D axis;
    float angle = 0.0f;
    q.getAxisAndAngle(&axis, &angle);
    EXPECT_GT(angle, 1.0f);
    EXPECT_GT(std::abs(axis.x()), std::abs(axis.y()));
}

TEST(ArcballTest, ReverseDragIsInverse) {
    QQuaternion fwd = arcball_rotation(QPointF(80, 90), QPointF(120, 110), QSize(200, 200));
    QQuaternion rev = arcball_rotation(QPointF(120, 110), QPointF(80, 90), QSize(200, 200));
    QQuaternion compose = (fwd * rev).normalized();
    EXPECT_NEAR(std::abs(compose.scalar()), 1.0f, 1e-3f);
    EXPECT_NEAR(compose.vector().length(), 0.0f, 1e-3f);
}

}  // namespace test
}  // namespace oe3dmol
