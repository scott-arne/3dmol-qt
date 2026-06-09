#include <gtest/gtest.h>

#include "oe3dmol/camera.h"

#include <cmath>

namespace oe3dmol {
namespace test {

TEST(CameraOrbitTest, ZoomToMatchesLegacyLookAtMatrix) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box, 45.0f);

    const QVector3D target = box.center();
    const float d = cam.distance();
    QMatrix4x4 legacy;
    legacy.lookAt(target + QVector3D(0, 0, d), target, QVector3D(0, 1, 0));

    QMatrix4x4 got = cam.view_matrix();
    const float* a = got.constData();
    const float* b = legacy.constData();
    for (int i = 0; i < 16; ++i) {
        EXPECT_NEAR(a[i], b[i], 1e-4f) << "element " << i;
    }
}

TEST(CameraOrbitTest, OrbitChangesViewButKeepsTargetDistance) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box, 45.0f);
    const float d0 = cam.distance();

    QMatrix4x4 before = cam.view_matrix();
    cam.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 90.0f));
    QMatrix4x4 after = cam.view_matrix();

    bool changed = false;
    for (int i = 0; i < 16; ++i)
        if (std::abs(before.constData()[i] - after.constData()[i]) > 1e-3f) changed = true;
    EXPECT_TRUE(changed);
    EXPECT_NEAR(cam.distance(), d0, 1e-5f);
    QVector3D vc = cam.view_matrix().map(box.center());
    EXPECT_NEAR(vc.x(), 0.0f, 1e-3f);
    EXPECT_NEAR(vc.y(), 0.0f, 1e-3f);
    EXPECT_NEAR(vc.z(), -d0, 1e-3f);
}

TEST(CameraOrbitTest, DollyScalesAndClampsDistance) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box, 45.0f);
    const float d0 = cam.distance();

    cam.dolly(0.5f);
    EXPECT_NEAR(cam.distance(), d0 * 0.5f, 1e-4f);

    cam.dolly(1e-9f);
    EXPECT_GT(cam.distance(), 0.0f);
}

TEST(CameraOrbitTest, PanRightMovesContentToPositiveViewX) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box, 45.0f);

    // pan(+dx) is a rightward "grab-drag": the original world origin ends up at
    // positive view-space x. Pins the sign convention the widget depends on.
    cam.pan(0.5f, 0.0f);
    QVector3D vc = cam.view_matrix().map(QVector3D(0, 0, 0));
    EXPECT_GT(vc.x(), 1e-3f);
}

TEST(CameraOrbitTest, DollyClampsAtMaxZoomOut) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);  // bounding_radius = sqrt(3)
    Camera cam;
    cam.zoom_to(box, 45.0f);

    cam.dolly(1e9f);  // extreme zoom-out must clamp at 1000 * scene_radius_
    // scene_radius_ = bounding_radius = sqrt(3) ~ 1.7320508.
    const float max_d = 1000.0f * 1.7320508f;
    EXPECT_LE(cam.distance(), max_d + 1.0f);
    EXPECT_GT(cam.distance(), max_d - 1.0f);  // actually reached the clamp
}

TEST(CameraOrbitTest, OrbitAccumulatesAcrossCalls) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box, 45.0f);

    // Two 45-degree Y orbits should equal one 90-degree Y orbit.
    cam.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 45.0f));
    cam.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 45.0f));
    QMatrix4x4 two_steps = cam.view_matrix();

    Camera cam2;
    cam2.zoom_to(box, 45.0f);
    cam2.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 90.0f));
    QMatrix4x4 one_step = cam2.view_matrix();

    for (int i = 0; i < 16; ++i) {
        EXPECT_NEAR(two_steps.constData()[i], one_step.constData()[i], 1e-3f)
            << "element " << i;
    }
}

}  // namespace test
}  // namespace oe3dmol
