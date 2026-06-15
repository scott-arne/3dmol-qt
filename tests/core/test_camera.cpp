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

TEST(CameraTest, ZoomToClampsExtremeFov) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);

    Camera cam;
    // fov=180 -> half-angle 90deg. In exact math tan(90deg)=inf; in float math
    // static_cast<float>(M_PI) rounds just past pi/2, so tan(half) is a huge but
    // FINITE value (~ -2e7). The pan scale 2*d*tan(half)/h therefore explodes the
    // target by millions of world units (and in the wrong direction) instead of
    // becoming inf/nan -- so a plain isfinite check cannot catch it. The clamp to
    // 170 keeps tan(half) at ~11.4, a sane finite scale.
    cam.zoom_to(box, 180.0f);

    EXPECT_TRUE(std::isfinite(cam.distance()));
    EXPECT_GT(cam.distance(), 0.0f);

    const QMatrix4x4 before = cam.view_matrix();
    cam.pan(10.0f, 10.0f, QSize(100, 100));
    const QMatrix4x4 after = cam.view_matrix();
    for (int i = 0; i < 16; ++i) {
        EXPECT_TRUE(std::isfinite(after.constData()[i]));
    }
    EXPECT_FALSE(qFuzzyCompare(before, after));  // pan still moves (clamp didn't freeze it)

    // The real guard: a 10px pan must shift the target by a sane, scene-scaled
    // amount, not by millions of world units. The view matrix encodes -target in
    // its translation, so the change in the translation column equals the target
    // shift. Bound it to a generous multiple of the scene size; an unclamped
    // fov=180 blows this up to ~1e7 and fails.
    const float shift_x = after(0, 3) - before(0, 3);
    const float shift_y = after(1, 3) - before(1, 3);
    const float shift_z = after(2, 3) - before(2, 3);
    const float shift = std::sqrt(shift_x * shift_x + shift_y * shift_y + shift_z * shift_z);
    const float scene_radius = box.bounding_radius();
    EXPECT_GT(shift, 0.0f);                  // pan moved the target
    EXPECT_LT(shift, 1000.0f * scene_radius);  // but not by an absurd amount
}

TEST(CameraTest, ZoomToClampsTinyFov) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);

    Camera cam;
    cam.zoom_to(box, 0.0f);  // fov->0: distance = radius/sin(fov/2) would diverge

    // The 1-degree floor keeps sin(fov/2) > 0, so distance stays finite (and
    // large but bounded) instead of diverging to infinity.
    EXPECT_TRUE(std::isfinite(cam.distance()));
    EXPECT_GT(cam.distance(), 0.0f);
    // 1-degree fov: distance = radius / sin(0.5deg) ~= 115 * radius for this unit
    // box (radius = sqrt(3) ~= 1.732); bound generously below the unclamped inf.
    EXPECT_LT(cam.distance(), 1.0e4f);
}

}  // namespace test
}  // namespace oe3dmol
