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

TEST(CameraOrbitTest, ZoomToFramesSceneInsideClipPlanesWithMargin) {
    // After framing, the whole bounding sphere (radius R, centered on the orbit
    // target) must sit STRICTLY inside the near/far clip planes -- with a margin,
    // not merely tangent. A tangent fit (near == D-R, far == D+R) leaves extreme
    // vertices exactly on the planes, where float MVP error + imposter billboard
    // slop flicker-clip them at some orbit angles. Require a real gap on both
    // sides so the scene never touches a plane.
    for (float r : {0.5f, 5.0f, 500.0f}) {  // tiny, normal, huge molecules
        BoundingBox box;
        box.include(QVector3D(0, 0, 0), r);  // bounding_radius = r*sqrt(3)
        Camera cam;
        cam.zoom_to(box, 45.0f);
        const float D = cam.distance();
        const float R = (box.max() - box.min()).length() * 0.5f;  // bounding sphere radius
        // Nearest/farthest scene point along the view axis (orbit-invariant).
        EXPECT_LT(cam.near_plane(), D - R)
            << "near plane not clear of the scene front (r=" << r << ")";
        EXPECT_GT(cam.far_plane(), D + R)
            << "far plane not clear of the scene back (r=" << r << ")";
        EXPECT_GT(cam.near_plane(), 0.0f) << "near must stay positive (r=" << r << ")";
        // Margin should scale with the scene, not be a fixed epsilon.
        EXPECT_GT((D - R) - cam.near_plane(), 0.01f * R)
            << "front margin too small to absorb float error (r=" << r << ")";
        EXPECT_GT(cam.far_plane() - (D + R), 0.01f * R)
            << "back margin too small to absorb float error (r=" << r << ")";
    }
}

TEST(CameraOrbitTest, FullOrbitNeverClipsFramedScene) {
    // The user's requirement: opening any molecule must allow a FULL rotation
    // from the initial framing with no front/back clipping. Build an asymmetric
    // box (so the extreme corner sweeps toward/away from the camera as it spins),
    // frame it, then sample many orbit orientations and assert every box corner
    // stays within (near, far) along the view axis with room to spare.
    BoundingBox box;
    box.include(QVector3D(-2.0f, -0.5f, -3.0f), 0.4f);
    box.include(QVector3D(4.0f, 1.0f, 2.0f), 0.4f);
    Camera cam;
    cam.zoom_to(box, 45.0f);

    const QVector3D mn = box.min(), mx = box.max();
    const QVector3D corners[8] = {
        {mn.x(), mn.y(), mn.z()}, {mn.x(), mn.y(), mx.z()},
        {mn.x(), mx.y(), mn.z()}, {mn.x(), mx.y(), mx.z()},
        {mx.x(), mn.y(), mn.z()}, {mx.x(), mn.y(), mx.z()},
        {mx.x(), mx.y(), mn.z()}, {mx.x(), mx.y(), mx.z()},
    };

    for (int ax = 0; ax < 3; ++ax) {
        const QVector3D axis(ax == 0 ? 1.f : 0.f, ax == 1 ? 1.f : 0.f, ax == 2 ? 1.f : 0.f);
        for (float ang = 0.0f; ang < 360.0f; ang += 15.0f) {
            Camera c = cam;  // copy the framing; only orientation changes
            c.orbit(QQuaternion::fromAxisAndAngle(axis, ang));
            const QMatrix4x4 view = c.view_matrix();
            for (const QVector3D& corner : corners) {
                const float view_z = view.map(corner).z();  // camera looks down -z
                const float depth = -view_z;                // positive distance from eye
                EXPECT_GT(depth, c.near_plane())
                    << "front clip at axis " << ax << " angle " << ang;
                EXPECT_LT(depth, c.far_plane())
                    << "back clip at axis " << ax << " angle " << ang;
            }
        }
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

    // Dragging right (+dx pixels) is a rightward "grab-drag": the original world
    // origin ends up at positive view-space x. Pins the sign convention the
    // widget depends on.
    cam.pan(40.0f, 0.0f, QSize(800, 600));
    QVector3D vc = cam.view_matrix().map(QVector3D(0, 0, 0));
    EXPECT_GT(vc.x(), 1e-3f);
}

TEST(CameraOrbitTest, PanFollowsCursorOnBothAxes) {
    // Pan is a grab-drag: the content under the cursor must follow the cursor
    // the SAME way on both axes. The widget hands the camera "rightward pixels"
    // as +dx and "upward pixels" as +dy. So a +dx drag must push a fixed world
    // point toward +NDC-x (screen right) and a +dy drag toward +NDC-y (screen
    // up). A vertical-sign bug (content moving down when you drag up) makes
    // diagonal drags track at a mirrored angle -- the "skewed pan" symptom.
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    const QSize vp(800, 600);
    auto ndc_of_origin = [&](const Camera& c) {
        QMatrix4x4 mvp = c.projection_matrix(static_cast<float>(vp.width()) /
                                             static_cast<float>(vp.height())) *
                         c.view_matrix();
        QVector4D clip = mvp * QVector4D(0, 0, 0, 1.0f);
        return QVector2D(clip.x() / clip.w(), clip.y() / clip.w());  // +y up
    };

    Camera cright;
    cright.zoom_to(box, 45.0f);
    cright.pan(40.0f, 0.0f, vp);  // drag right
    EXPECT_GT(ndc_of_origin(cright).x(), 1e-3f) << "drag right: content should go screen-right";

    Camera cup;
    cup.zoom_to(box, 45.0f);
    cup.pan(0.0f, 40.0f, vp);  // drag up
    EXPECT_GT(ndc_of_origin(cup).y(), 1e-3f) << "drag up: content should go screen-up";
}

TEST(CameraOrbitTest, PanUsesCurrentViewAfterOrbit) {
    // The user's hypothesis: pan might act w.r.t. a previous camera view. pan()
    // reads the live orientation, so after orbiting, dragging right/up must
    // STILL move content screen-right/up. Track a fixed world point's NDC.
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    const QSize vp(800, 600);
    const QVector3D probe(0.3f, 0.0f, 0.0f);  // off-center so orbit changes its view pos
    auto ndc = [&](const Camera& c) {
        QMatrix4x4 mvp = c.projection_matrix(static_cast<float>(vp.width()) /
                                             static_cast<float>(vp.height())) *
                         c.view_matrix();
        QVector4D clip = mvp * QVector4D(probe, 1.0f);
        return QVector2D(clip.x() / clip.w(), clip.y() / clip.w());
    };

    Camera cam;
    cam.zoom_to(box, 45.0f);
    cam.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 90.0f));
    cam.orbit(QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 30.0f));

    const QVector2D before = ndc(cam);
    cam.pan(40.0f, 0.0f, vp);  // drag right
    EXPECT_GT(ndc(cam).x() - before.x(), 1e-3f) << "after orbit, drag right -> screen-right";

    const QVector2D mid = ndc(cam);
    cam.pan(0.0f, 40.0f, vp);  // drag up
    EXPECT_GT(ndc(cam).y() - mid.y(), 1e-3f) << "after orbit, drag up -> screen-up";
}

TEST(CameraOrbitTest, PanIsAspectIndependentAndCursorScaled) {
    // The skew bug: panning used to divide x by width and y by height, so on a
    // non-square viewport an equal pixel drag in x vs y moved content by
    // different world amounts (diagonal drags tracked off-axis). Pixels are
    // square, so equal pixel deltas must produce equal world shifts, and the
    // shift must equal the true world-units-per-pixel at the target plane
    // (2*distance*tan(fov/2)/height) so the content tracks the cursor 1:1.
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    const QSize vp(800, 600);  // deliberately non-square to expose any skew
    const float drag = 20.0f;  // pixels

    Camera cx;
    cx.zoom_to(box, 45.0f);
    cx.pan(drag, 0.0f, vp);
    const float vx = cx.view_matrix().map(QVector3D(0, 0, 0)).x();

    Camera cy;
    cy.zoom_to(box, 45.0f);
    cy.pan(0.0f, drag, vp);
    const float vy = cy.view_matrix().map(QVector3D(0, 0, 0)).y();

    // No aspect skew: equal pixels in x and y -> equal-magnitude world shift.
    EXPECT_NEAR(std::abs(vx), std::abs(vy), 1e-3f);

    // Cursor-scaled: shift equals drag pixels * world-units-per-pixel.
    const float half_fov = (45.0f * 0.5f) * static_cast<float>(M_PI) / 180.0f;
    const float world_per_px = 2.0f * cx.distance() * std::tan(half_fov) / 600.0f;
    EXPECT_NEAR(std::abs(vx), drag * world_per_px, 1e-3f);
}

TEST(CameraOrbitTest, MoveZChangesDistanceButHoldsClipPlanes) {
    // Middle-drag Z move for clipping tests: it must change the camera distance
    // (so the near plane can slice into the molecule) WITHOUT re-deriving the
    // clip planes. projection_matrix depends only on fov/aspect/near/far -- not
    // distance -- so holding near/far fixed means the projection is unchanged
    // after move_z (unlike dolly, which re-fits the clip planes).
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box, 45.0f);
    const float d0 = cam.distance();
    const QMatrix4x4 proj_before = cam.projection_matrix(1.0f);

    // Drag up (negative screen-y delta) pushes the camera toward the scene.
    cam.move_z(-60.0f, QSize(800, 600));
    const float d_in = cam.distance();
    EXPECT_LT(d_in, d0) << "drag up should move the camera toward the scene";

    const QMatrix4x4 proj_after = cam.projection_matrix(1.0f);
    for (int i = 0; i < 16; ++i) {
        EXPECT_NEAR(proj_before.constData()[i], proj_after.constData()[i], 1e-5f)
            << "clip planes must stay fixed during move_z (element " << i << ")";
    }

    // Drag down reverses direction: the camera moves back outward. (The step is
    // distance-proportional like pan/dolly, so it need not land exactly on d0.)
    cam.move_z(60.0f, QSize(800, 600));
    EXPECT_GT(cam.distance(), d_in) << "drag down should move the camera back out";
}

TEST(CameraOrbitTest, MoveZKeepsDistancePositive) {
    BoundingBox box;
    box.include(QVector3D(0.0f, 0.0f, 0.0f), 1.0f);
    Camera cam;
    cam.zoom_to(box, 45.0f);

    // A huge forward push must not drive distance to zero or negative.
    cam.move_z(-1e6f, QSize(800, 600));
    EXPECT_GT(cam.distance(), 0.0f);
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
