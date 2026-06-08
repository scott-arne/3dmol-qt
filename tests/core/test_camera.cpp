#include <gtest/gtest.h>

#include "oe3dmol/camera.h"
#include "oe3dmol/atom_data.h"

#include <cmath>

namespace oe3dmol {
namespace test {

TEST(CameraTest, ZoomToCentersTarget) {
    BoundingBox box;
    box.include(QVector3D(-1.0f, -1.0f, -1.0f));
    box.include(QVector3D(1.0f, 1.0f, 1.0f));

    Camera cam;
    cam.zoom_to(box, 45.0f);

    QVector3D view_center = cam.view_matrix().map(QVector3D(0.0f, 0.0f, 0.0f));
    EXPECT_NEAR(view_center.x(), 0.0f, 1e-4f);
    EXPECT_NEAR(view_center.y(), 0.0f, 1e-4f);
    EXPECT_LT(view_center.z(), 0.0f);  // in front of camera (-Z)
}

TEST(CameraTest, DistanceFitsBoundingSphereInFov) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);

    Camera cam;
    cam.zoom_to(box, 45.0f);

    // BoundingBox::bounding_radius() is the half-diagonal of the box, so a
    // sphere of radius 1.0 at the origin yields a bounding radius of sqrt(3).
    const float bounding_radius = box.bounding_radius();
    float expected = bounding_radius / std::sin(static_cast<float>(22.5 * M_PI / 180.0));
    EXPECT_NEAR(cam.distance(), expected, 1e-3f);
}

TEST(CameraTest, ProjectionIsFinite) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box);
    QMatrix4x4 p = cam.projection_matrix(1.0f);
    EXPECT_TRUE(std::isfinite(p(0, 0)));
    EXPECT_GT(p(0, 0), 0.0f);
}

}  // namespace test
}  // namespace oe3dmol
