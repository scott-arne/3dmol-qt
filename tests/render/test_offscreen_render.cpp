#include <gtest/gtest.h>

#include <cstdlib>
#include <cmath>

#include "oe3dmol/rhi_context.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/sphere_imposter_pipeline.h"
#include "oe3dmol/sphere_imposter_builder.h"
#include "oe3dmol/camera.h"
#include "oe3dmol/cylinder_pipeline.h"
#include "oe3dmol/cylinder_builder.h"
#include "oe3dmol/bond_data.h"
#include "oe3dmol/viewer.h"
#include "oe3dmol/scene_renderer.h"
#include "oe3dmol/mol_adapter.h"
#include "oe3dmol/bond_adapter.h"

#include <oechem.h>

#include <QGuiApplication>
#include <QImage>
#include <QQuaternion>

#include <rhi/qrhi.h>

namespace oe3dmol {
namespace test {

// A QGuiApplication is required for QRhi backends. Created once per process.
class RenderEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        static int argc = 1;
        static char arg0[] = "oe3dmol_render_tests";
        static char* argv[] = {arg0, nullptr};
        app_ = new QGuiApplication(argc, argv);
    }
    void TearDown() override { delete app_; }
private:
    QGuiApplication* app_ = nullptr;
};

::testing::Environment* const kEnv =
    ::testing::AddGlobalTestEnvironment(new RenderEnvironment);

TEST(RhiContextTest, ClearsToBackgroundColor) {
    RhiContext ctx(QSize(64, 64));
    QImage img = ctx.render_frame(QColor(10, 20, 30), [](QRhiCommandBuffer*) {});
    ASSERT_FALSE(img.isNull());
    EXPECT_EQ(img.width(), 64);
    EXPECT_EQ(img.height(), 64);
    QColor center = img.pixelColor(32, 32);
    EXPECT_NEAR(center.red(), 10, 2);
    EXPECT_NEAR(center.green(), 20, 2);
    EXPECT_NEAR(center.blue(), 30, 2);
}

TEST(SphereImposterPipelineTest, DrawsRedSphereAtCenter) {
    const QSize sz(128, 128);
    RhiContext ctx(sz);

    AtomData atoms;
    atoms.add(QVector3D(0.0f, 0.0f, 0.0f), 1.0f, Color(255, 0, 0), 0u);
    MeshBuffer mesh = build_sphere_imposters(atoms);

    Camera cam;
    cam.zoom_to(atoms.bounds);

    SphereImposterPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());

    ImposterUniforms u;
    u.view = cam.view_matrix();
    u.projection = cam.projection_matrix(1.0f);
    u.model_view = u.view;  // model is identity

    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* batch = ctx.rhi()->nextResourceUpdateBatch();
        pipeline.upload(mesh, batch);
        pipeline.set_uniforms(u, batch);
        cb->resourceUpdate(batch);
        pipeline.draw(cb, sz);
    });

    ASSERT_FALSE(img.isNull());
    QColor center = img.pixelColor(64, 64);
    EXPECT_GT(center.red(), 80);
    EXPECT_LT(center.green(), 60);
    EXPECT_LT(center.blue(), 60);

    QColor corner = img.pixelColor(2, 2);
    EXPECT_LT(corner.red(), 20);
}

TEST(SphereImposterPipelineTest, OpacityDarkensOverBlackBackground) {
    const QSize sz(128, 128);
    auto render_alpha = [&](float alpha) -> QColor {
        RhiContext ctx(sz);
        AtomData atoms;
        Color red(255, 0, 0);
        red.setAlphaF(alpha);
        atoms.add(QVector3D(0, 0, 0), 1.0f, red, 0u);
        MeshBuffer mesh = build_sphere_imposters(atoms);
        Camera cam;
        cam.zoom_to(atoms.bounds);
        SphereImposterPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());
        ImposterUniforms u;
        u.view = cam.view_matrix();
        u.projection = cam.projection_matrix(1.0f);
        u.model_view = u.view;
        QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
            QRhiResourceUpdateBatch* b = ctx.rhi()->nextResourceUpdateBatch();
            pipeline.upload(mesh, b);
            pipeline.set_uniforms(u, b);
            cb->resourceUpdate(b);
            pipeline.draw(cb, sz);
        });
        return img.pixelColor(64, 64);
    };
    QColor opaque = render_alpha(1.0f);
    QColor semi = render_alpha(0.5f);
    // Opaque center is bright red.
    EXPECT_GT(opaque.red(), 150);
    // Semi-transparent center over black is materially dimmer (alpha^2=0.25 of opaque)...
    EXPECT_LT(semi.red(), opaque.red() - 30);
    // ...but still clearly rendered (not background black).
    EXPECT_GT(semi.red(), 10);
}

TEST(CylinderPipelineTest, DrawsHorizontalBond) {
    const QSize sz(128, 128);
    RhiContext ctx(sz);

    BondData bonds;
    bonds.add(QVector3D(-1, 0, 0), QVector3D(1, 0, 0), 0.25f, Color(255, 0, 0), 0u);
    MeshBuffer bb = build_cylinder_billboard();
    std::vector<float> inst = build_cylinder_instances(bonds);

    Camera cam;
    cam.zoom_to(bonds.bounds);

    CylinderPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());
    ImposterUniforms u;
    u.view = cam.view_matrix();
    u.projection = cam.projection_matrix(1.0f);
    u.model_view = u.view;

    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* batch = ctx.rhi()->nextResourceUpdateBatch();
        pipeline.upload(bb, inst, batch);
        pipeline.set_uniforms(u, batch);
        cb->resourceUpdate(batch);
        pipeline.draw(cb, sz);
    });

    ASSERT_FALSE(img.isNull());
    QColor center = img.pixelColor(64, 64);
    EXPECT_GT(center.red(), 60);
    EXPECT_LT(center.green(), 60);
    EXPECT_LT(center.blue(), 60);
    QColor corner = img.pixelColor(2, 2);
    EXPECT_LT(corner.red(), 20);
}

TEST(CylinderPipelineTest, ZeroInstancesIsSafeNoOp) {
    const QSize sz(64, 64);
    RhiContext ctx(sz);
    MeshBuffer bb = build_cylinder_billboard();
    std::vector<float> inst;  // empty
    CylinderPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());
    ImposterUniforms u;
    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* batch = ctx.rhi()->nextResourceUpdateBatch();
        pipeline.upload(bb, inst, batch);
        pipeline.set_uniforms(u, batch);
        cb->resourceUpdate(batch);
        pipeline.draw(cb, sz);
    });
    ASSERT_FALSE(img.isNull());
    EXPECT_LT(img.pixelColor(32, 32).red(), 20);
}

TEST(CylinderPipelineTest, TwoBondsRenderDistinctPerInstanceColors) {
    const QSize sz(192, 192);
    RhiContext ctx(sz);

    // Two well-separated horizontal bonds: a RED one in the upper half (y=+2),
    // a BLUE one in the lower half (y=-2). If per-instance data is mis-indexed
    // (e.g. both draw instance 0), we'd see the same color in both regions.
    BondData bonds;
    bonds.add(QVector3D(-1, 2, 0), QVector3D(1, 2, 0), 0.3f, Color(255, 0, 0), 0u);
    bonds.add(QVector3D(-1, -2, 0), QVector3D(1, -2, 0), 0.3f, Color(0, 0, 255), 1u);
    MeshBuffer bb = build_cylinder_billboard();
    std::vector<float> inst = build_cylinder_instances(bonds);
    ASSERT_EQ(inst.size(), 22u);  // 2 instances * 11 floats

    Camera cam;
    cam.zoom_to(bonds.bounds);

    CylinderPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());
    ImposterUniforms u;
    u.view = cam.view_matrix();
    u.projection = cam.projection_matrix(1.0f);
    u.model_view = u.view;

    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* batch = ctx.rhi()->nextResourceUpdateBatch();
        pipeline.upload(bb, inst, batch);
        pipeline.set_uniforms(u, batch);
        cb->resourceUpdate(batch);
        pipeline.draw(cb, sz);
    });
    ASSERT_FALSE(img.isNull());

    // Scan the upper half for red-dominant pixels and the lower half for
    // blue-dominant pixels. With camera looking down -Z and +Y up in view space,
    // world +Y maps to the upper part of the image (smaller y pixel coords).
    bool red_top = false, blue_bottom = false;
    bool red_bottom = false, blue_top = false;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor c = img.pixelColor(x, y);
            const bool isRed = c.red() > 120 && c.green() < 80 && c.blue() < 80;
            const bool isBlue = c.blue() > 120 && c.red() < 80 && c.green() < 80;
            if (y < img.height() / 2) {
                if (isRed) red_top = true;
                if (isBlue) blue_top = true;
            } else {
                if (isRed) red_bottom = true;
                if (isBlue) blue_bottom = true;
            }
        }
    }
    // Both colors must appear, in OPPOSITE halves (proving per-instance indexing).
    EXPECT_TRUE(red_top || red_bottom);    // red rendered somewhere
    EXPECT_TRUE(blue_top || blue_bottom);  // blue rendered somewhere
    EXPECT_NE(red_top, blue_top);          // the two halves differ in color
}

TEST(ViewerCompositionTest, BallAndStickDrawsBothSphereAndStick) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double ca[3] = {-0.7, 0, 0};
    double co[3] = {0.7, 0, 0};
    mol.SetCoords(a, ca);
    mol.SetCoords(o, co);
    mol.NewBond(a, o, 1);
    const OEChem::OEMolBase& base = mol;

    Viewer viewer(200, 200);
    viewer.set_background_color(QColor(0, 0, 0));
    int idx = viewer.add_model(&base);
    viewer.set_style(idx, std::vector<StyleSpec>{
        SphereStyle{}.scale(0.3), StickStyle{}});
    QImage img = viewer.render();

    ASSERT_FALSE(img.isNull());
    int nonblack = 0;
    for (int y = 0; y < img.height(); y += 4)
        for (int x = 0; x < img.width(); x += 4)
            if (img.pixelColor(x, y).red() > 20 || img.pixelColor(x, y).green() > 20 ||
                img.pixelColor(x, y).blue() > 20)
                ++nonblack;
    EXPECT_GT(nonblack, 50);
    bool red_right = false;
    for (int y = 0; y < img.height(); y += 2)
        for (int x = img.width() / 2; x < img.width(); x += 2) {
            QColor c = img.pixelColor(x, y);
            if (c.red() > 120 && c.green() < 80 && c.blue() < 80) red_right = true;
        }
    EXPECT_TRUE(red_right);
}

TEST(ViewerCompositionTest, StickOnlyDrawsSticksNoSpheresPath) {
    // A stick-only style exercises the has_spheres==false branch of render().
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double ca[3] = {-0.7, 0, 0};
    double co[3] = {0.7, 0, 0};
    mol.SetCoords(a, ca);
    mol.SetCoords(o, co);
    mol.NewBond(a, o, 1);
    const OEChem::OEMolBase& base = mol;

    Viewer viewer(160, 160);
    viewer.set_background_color(QColor(0, 0, 0));
    int idx = viewer.add_model(&base);
    viewer.set_style(idx, std::vector<StyleSpec>{StickStyle{}});
    QImage img = viewer.render();

    ASSERT_FALSE(img.isNull());
    int nonblack = 0;
    for (int y = 0; y < img.height(); y += 4)
        for (int x = 0; x < img.width(); x += 4) {
            QColor c = img.pixelColor(x, y);
            if (c.red() > 20 || c.green() > 20 || c.blue() > 20) ++nonblack;
        }
    EXPECT_GT(nonblack, 10);  // sticks rendered
}

TEST(ViewerCompositionTest, Sp1SphereStyleOverloadStillRenders) {
    // Regression: the single-SphereStyle overload (delegating to the vector form)
    // must still render after the multi-style refactor.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    double ca[3] = {0, 0, 0};
    mol.SetCoords(a, ca);
    const OEChem::OEMolBase& base = mol;

    Viewer viewer(160, 160);
    viewer.set_background_color(QColor(0, 0, 0));
    int idx = viewer.add_model(&base);
    viewer.set_style(idx, SphereStyle{});  // SP1 overload
    QImage img = viewer.render();

    ASSERT_FALSE(img.isNull());
    int nonblack = 0;
    for (int y = 0; y < img.height(); y += 4)
        for (int x = 0; x < img.width(); x += 4) {
            QColor c = img.pixelColor(x, y);
            if (c.red() > 20 || c.green() > 20 || c.blue() > 20) ++nonblack;
        }
    EXPECT_GT(nonblack, 10);  // sphere rendered via SP1 overload
}

TEST(SceneRendererTest, RendersSphereSceneNonEmpty) {
    const QSize sz(128, 128);
    RhiContext ctx(sz);

    AtomData atoms;
    atoms.add(QVector3D(0, 0, 0), 1.0f, Color(255, 0, 0), 0u);
    BondData bonds;  // empty

    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(atoms, bonds);

    Camera cam;
    cam.zoom_to(atoms.bounds);

    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());
    QColor center = img.pixelColor(64, 64);
    EXPECT_GT(center.red(), 60);
}

TEST(SceneRendererTest, OrbitChangesRenderedImage) {
    const QSize sz(128, 128);

    AtomData atoms;
    atoms.add(QVector3D(-1.5f, 0, 0), 0.6f, Color(255, 0, 0), 0u);   // red left
    atoms.add(QVector3D(1.5f, 0, 0), 0.6f, Color(0, 0, 255), 1u);    // blue right
    BondData bonds;

    auto render_with = [&](const Camera& cam) {
        RhiContext ctx(sz);
        SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
        renderer.set_geometry(atoms, bonds);
        return ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
            renderer.draw(cb, cam, sz);
        });
    };

    Camera cam0;
    cam0.zoom_to(atoms.bounds);
    QImage img0 = render_with(cam0);

    Camera cam1;
    cam1.zoom_to(atoms.bounds);
    cam1.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 90.0f));
    QImage img1 = render_with(cam1);

    ASSERT_FALSE(img0.isNull());
    ASSERT_FALSE(img1.isNull());
    int diff = 0;
    for (int y = 0; y < sz.height(); y += 4)
        for (int x = 0; x < sz.width(); x += 4)
            if (img0.pixelColor(x, y) != img1.pixelColor(x, y)) ++diff;
    EXPECT_GT(diff, 5);
}

TEST(SceneRendererTest, SphereStaysLitAfter180Orbit) {
    // Imposter normals are reconstructed in view space and always face the
    // camera, so the directional light must be a view-space HEADLIGHT (fixed
    // relative to the camera). If it is instead rotated by the view matrix
    // (world-fixed), orbiting 180 degrees flips the light behind the
    // camera-facing surface and the whole molecule renders black. A sphere
    // centered on the orbit target occupies the same center pixels at every
    // orbit angle, so its center brightness must not collapse when we spin
    // around.
    const QSize sz(128, 128);
    AtomData atoms;
    atoms.add(QVector3D(0, 0, 0), 1.0f, Color(220, 220, 220), 0u);
    BondData bonds;

    auto center_lum = [&](const Camera& cam) {
        RhiContext ctx(sz);
        SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
        renderer.set_geometry(atoms, bonds);
        QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
            renderer.draw(cb, cam, sz);
        });
        EXPECT_FALSE(img.isNull());
        return img.pixelColor(64, 64).red();
    };

    Camera cam0;
    cam0.zoom_to(atoms.bounds);
    Camera cam180;
    cam180.zoom_to(atoms.bounds);
    cam180.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 180.0f));

    EXPECT_GT(center_lum(cam0), 60) << "sphere should be lit at orbit 0";
    EXPECT_GT(center_lum(cam180), 60)
        << "sphere went dark after a 180-degree orbit (light is not a headlight)";
}

TEST(SceneRendererTest, StickStaysLitAfter180Orbit) {
    // Same headlight invariant for the cylinder imposter. A stick through the
    // orbit target still crosses screen-center after any orbit, so the center
    // pixel stays on the cylinder body; its brightness must not collapse at 180.
    const QSize sz(128, 128);
    AtomData atoms;  // sticks only
    BondData bonds;
    bonds.add(QVector3D(-1, 0, 0), QVector3D(1, 0, 0), 0.4f, Color(0, 220, 0), 0u);

    auto center_green = [&](const Camera& cam) {
        RhiContext ctx(sz);
        SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
        renderer.set_geometry(atoms, bonds);
        QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
            renderer.draw(cb, cam, sz);
        });
        EXPECT_FALSE(img.isNull());
        return img.pixelColor(64, 64).green();
    };

    Camera cam0;
    cam0.zoom_to(bonds.bounds);
    Camera cam180;
    cam180.zoom_to(bonds.bounds);
    cam180.orbit(QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 180.0f));

    EXPECT_GT(center_green(cam0), 40) << "stick should be lit at orbit 0";
    EXPECT_GT(center_green(cam180), 40)
        << "stick went dark after a 180-degree orbit (light is not a headlight)";
}

TEST(SceneRendererTest, RendersStickSceneNonEmpty) {
    const QSize sz(128, 128);
    RhiContext ctx(sz);

    // A single horizontal half-bond (cylinder path; no spheres).
    AtomData atoms;  // empty
    BondData bonds;
    bonds.add(QVector3D(-1, 0, 0), QVector3D(1, 0, 0), 0.3f, Color(0, 255, 0), 0u);

    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(atoms, bonds);

    Camera cam;
    cam.zoom_to(bonds.bounds);

    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());
    QColor center = img.pixelColor(64, 64);
    EXPECT_GT(center.green(), 40);  // green cylinder rendered
}

TEST(SceneRendererTest, SetGeometryReuploadsOnSecondDraw) {
    const QSize sz(128, 128);
    RhiContext ctx(sz);

    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());

    // First geometry: a red sphere.
    AtomData a1;
    a1.add(QVector3D(0, 0, 0), 1.0f, Color(255, 0, 0), 0u);
    renderer.set_geometry(a1, BondData{});
    Camera cam1;
    cam1.zoom_to(a1.bounds);
    QImage img1 = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam1, sz);
    });

    // Second geometry on the SAME renderer: a blue sphere. Must re-upload.
    AtomData a2;
    a2.add(QVector3D(0, 0, 0), 1.0f, Color(0, 0, 255), 0u);
    renderer.set_geometry(a2, BondData{});
    Camera cam2;
    cam2.zoom_to(a2.bounds);
    QImage img2 = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam2, sz);
    });

    ASSERT_FALSE(img1.isNull());
    ASSERT_FALSE(img2.isNull());
    // The re-upload took effect: center went from red-dominant to blue-dominant.
    QColor c1 = img1.pixelColor(64, 64);
    QColor c2 = img2.pixelColor(64, 64);
    EXPECT_GT(c1.red(), c1.blue());
    EXPECT_GT(c2.blue(), c2.red());
}

TEST(SceneRendererTest, CylinderCapBuriedInSphereIsOccluded) {
    const QSize sz(256, 256);
    RhiContext ctx(sz);

    // A RED sphere at the origin (radius 0.8). A GREEN half-bond whose START is
    // the atom center (0,0,0) -- its flat cap is therefore buried INSIDE the
    // sphere -- extending toward the camera (+Z) and to the side. With correct
    // depth ordering the sphere's front surface occludes the buried cap, so the
    // sphere center reads RED. With the bug, the green cap floats in front -> green.
    AtomData atoms;
    atoms.add(QVector3D(0, 0, 0), 0.8f, Color(255, 0, 0), 0u);
    BondData bonds;
    bonds.add(QVector3D(0, 0, 0), QVector3D(0.5f, 0.0f, 0.5f), 0.3f,
              Color(0, 255, 0), 0u);

    BoundingBox bounds;
    bounds.include(QVector3D(0, 0, 0), 0.8f);
    bounds.include(QVector3D(0.5f, 0.0f, 0.5f), 0.3f);

    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(atoms, bonds);

    Camera cam;
    cam.zoom_to(bounds);

    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());

    // The dead-center pixel projects onto the sphere's front surface (the bond
    // axis goes toward +Z/+X, so the cap is buried behind the front of the ball
    // at center). It must read RED (sphere), not GREEN (buried cap bleeding through).
    QColor center = img.pixelColor(128, 128);
    EXPECT_GT(center.red(), 80) << "expected red sphere surface at center";
    EXPECT_LT(center.green(), 80)
        << "green cylinder cap is bleeding through the opaque sphere (depth bug)";
}

TEST(SceneRendererTest, EmptySceneRendersClearedFrame) {
    const QSize sz(64, 64);
    RhiContext ctx(sz);

    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    // No set_geometry call: nothing to draw.
    Camera cam;  // default

    QImage img = ctx.render_frame(QColor(10, 20, 30), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());
    QColor c = img.pixelColor(32, 32);
    EXPECT_NEAR(c.red(), 10, 4);
    EXPECT_NEAR(c.green(), 20, 4);
    EXPECT_NEAR(c.blue(), 30, 4);
}

TEST(CylinderPipelineTest, EndCapIsRounded) {
    // A single short horizontal bond on black; the right end should be a convex
    // (rounded) cap, not a flat vertical edge. Sample columns near the right end:
    // a rounded cap's filled height shrinks smoothly to zero; a flat cap drops
    // from full height to zero in one column.
    const QSize sz(256, 128);
    RhiContext ctx(sz);
    BondData bonds;
    bonds.add(QVector3D(-0.6f, 0, 0), QVector3D(0.6f, 0, 0), 0.3f, Color(0, 255, 0), 0u);
    MeshBuffer bb = build_cylinder_billboard();
    std::vector<float> inst = build_cylinder_instances(bonds);
    Camera cam; cam.zoom_to(bonds.bounds);
    CylinderPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());
    ImposterUniforms u;
    u.view = cam.view_matrix();
    u.projection = cam.projection_matrix(static_cast<float>(sz.width()) / sz.height());
    u.model_view = u.view;
    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* b = ctx.rhi()->nextResourceUpdateBatch();
        pipeline.upload(bb, inst, b);
        pipeline.set_uniforms(u, b);
        cb->resourceUpdate(b);
        pipeline.draw(cb, sz);
    });
    ASSERT_FALSE(img.isNull());
    // Find the rightmost column containing any green, and a column 6px left of it.
    auto green_height = [&](int x) {
        int h = 0;
        for (int y = 0; y < sz.height(); ++y) {
            QColor c = img.pixelColor(x, y);
            if (c.green() > 80 && c.red() < 80) ++h;
        }
        return h;
    };
    int rightmost = -1;
    for (int x = sz.width() - 1; x >= 0; --x) { if (green_height(x) > 0) { rightmost = x; break; } }
    ASSERT_GT(rightmost, 10);
    // Rounded cap: the edge column has much less filled height than 6px inward.
    EXPECT_LT(green_height(rightmost), green_height(rightmost - 6))
        << "end cap is flat, not rounded";
}

TEST(SceneRendererTest, BentBondJointHasNoHoleAndNoSeam) {
    const QSize sz(256, 256);
    RhiContext ctx(sz);
    // Bent path A-B-C (~120 deg) as sticks only, via two half-bonds per bond.
    AtomData atoms;  // sticks only: no atom spheres
    BondData bonds;
    QVector3D a(-1.0f, 0.3f, 0), b(0.0f, -0.3f, 0), c(1.0f, 0.3f, 0);
    QVector3D mid_ab = (a + b) * 0.5f, mid_bc = (b + c) * 0.5f;
    Color grey(144, 144, 144);
    bonds.add(a, mid_ab, 0.25f, grey, 0u);
    bonds.add(b, mid_ab, 0.25f, grey, 0u);  // half-bonds meeting at B's first bond
    bonds.add(b, mid_bc, 0.25f, grey, 1u);
    bonds.add(c, mid_bc, 0.25f, grey, 1u);
    BoundingBox bounds;
    for (const QVector3D& p : {a, b, c, mid_ab, mid_bc}) bounds.include(p, 0.25f);

    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(atoms, bonds);
    Camera cam; cam.zoom_to(bounds);
    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());

    // Project vertex B to screen and inspect a square around it. Within the bond
    // region near B there must be NO background-black hole (the imposter caps
    // fill it), and the grey values must be continuous (no sharp seam step).
    QVector3D bview = cam.view_matrix().map(b);
    QMatrix4x4 proj = cam.projection_matrix(1.0f);
    QVector4D clip4 = proj * QVector4D(bview, 1.0f);
    QVector3D clip = clip4.toVector3D() / clip4.w();  // perspective divide
    int bx = static_cast<int>((clip.x() * 0.5f + 0.5f) * sz.width());
    int by = static_cast<int>((1.0f - (clip.y() * 0.5f + 0.5f)) * sz.height());

    int grey_px = 0, black_px = 0;
    for (int dy = -6; dy <= 6; ++dy) {
        for (int dx = -6; dx <= 6; ++dx) {
            int x = bx + dx, y = by + dy;
            if (x < 0 || y < 0 || x >= sz.width() || y >= sz.height()) continue;
            QColor cc = img.pixelColor(x, y);
            bool blackish = cc.red() < 25 && cc.green() < 25 && cc.blue() < 25;
            if (blackish) ++black_px; else ++grey_px;
        }
    }
    EXPECT_GT(grey_px, 0) << "joint region empty";
    EXPECT_EQ(black_px, 0) << "background hole at the bond joint (caps not filling)";

    // Seam-continuity check: scan a horizontal line through the joint (row y=by)
    // and measure the largest green-channel step between ADJACENT on-surface
    // pixels. The molecule is flat grey under one-sided Lambert shading, so the
    // rendered grey varies smoothly across a seamless imposter. A visible seam
    // (a separate sphere capping the stick) would show up as a sudden step of
    // tens of levels where the body meets the cap. Only consider pixel pairs
    // where BOTH pixels are on the surface (green >= 25) so the legitimate
    // step down to the black background (covered by the hole test above) is
    // excluded.
    int max_adjacent_delta = 0;
    int prev_green = -1;
    for (int x = bx - 12; x <= bx + 12; ++x) {
        if (x < 0 || x >= sz.width() || by < 0 || by >= sz.height()) {
            prev_green = -1;
            continue;
        }
        int g = img.pixelColor(x, by).green();
        if (g < 25) {  // background pixel: break the adjacency chain
            prev_green = -1;
            continue;
        }
        if (prev_green >= 0) {
            int delta = std::abs(g - prev_green);
            if (delta > max_adjacent_delta) max_adjacent_delta = delta;
        }
        prev_green = g;
    }
    // Observed seamless max-adjacent-delta on Metal: 1 (greens vary smoothly
    // 141..144 across the joint). Threshold 20 tolerates smooth Lambert
    // variation + anti-aliasing while catching a real seam (a sudden
    // ~tens-of-levels shading step at a body->cap boundary).
    EXPECT_LT(max_adjacent_delta, 20)
        << "shading discontinuity (seam) at joint: " << max_adjacent_delta;
}


TEST(SceneRendererTest, FrontHalfGeometryIsNotClipped) {
    // Regression for the front-clipping bug: gl_Position must be mapped into the
    // backend's clip volume via QRhi::clipSpaceCorrMatrix(). We feed an OpenGL-
    // convention projection (clip z in [-1,1]); Metal/D3D/Vulkan clip at z<0, so
    // WITHOUT the correction the entire front half of the frustum is discarded
    // and geometry that orbits toward the camera vanishes.
    //
    // Frame a box centered at the origin, then place a RED sphere toward the
    // camera (+Z). With bounds spanning z in [-4,4] and a 45-degree fovy, the
    // sphere at z=+3 sits at view depth ~8.1 (near ~6.6, distance ~11.1), i.e.
    // the front half where GL-convention ndc z is negative -> clipped by the bug.
    // It projects to screen center and must render.
    const QSize sz(128, 128);
    RhiContext ctx(sz);
    AtomData atoms;
    atoms.add(QVector3D(0, 0, 3.0f), 1.0f, Color(255, 0, 0), 0u);  // front, toward camera
    BondData bonds;
    BoundingBox bounds;
    bounds.include(QVector3D(0, 0, 3.0f), 1.0f);
    bounds.include(QVector3D(0, 0, -3.0f), 1.0f);  // counterweight: center ~ origin

    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(atoms, bonds);
    Camera cam; cam.zoom_to(bounds);

    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());
    QColor center = img.pixelColor(64, 64);
    EXPECT_GT(center.red(), 80)
        << "front-half sphere was clipped (missing clip-space correction)";
}




TEST(CylinderPipelineTest, TiltedBondCapStaysRounded) {
    // Regression for flat caps on tilted bonds: the billboard used to extend past
    // each endpoint along the 3D bond axis, which foreshortens as the bond tilts
    // toward the camera, so the quad no longer covered the hemispherical cap's
    // screen disc and the near cap rendered as a flat slice. A cap is a hemisphere
    // of radius r centered on the endpoint -- its silhouette is a disc of radius r
    // in the screen plane regardless of tilt -- so the cap end must be CONVEX
    // (rounded), never a straight horizontal edge, at any tilt.
    //
    // Bond runs from the origin straight up the screen (+Y) but tilted strongly
    // toward the camera (+Z). The near (bottom) endpoint's cap is the one that
    // used to flatten. Sample rows near the bottom of the rendered shape: a
    // rounded cap's filled WIDTH shrinks smoothly toward the tip; a flat slice
    // keeps full width then drops to zero in one row.
    const QSize sz(300, 300);
    BondData bonds;
    bonds.add(QVector3D(0, 0, 0), QVector3D(0.0f, 1.0f, 1.2f), 0.35f, Color(80, 80, 220), 0u);
    BoundingBox bounds;
    bounds.include(QVector3D(0, 0, 0), 0.35f);
    bounds.include(QVector3D(0.0f, 1.0f, 1.2f), 0.35f);
    AtomData none;
    RhiContext ctx(sz);
    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(none, bonds);
    Camera cam; cam.zoom_to(bounds);
    QImage img = ctx.render_frame(QColor(255, 255, 255), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());

    auto filled_width = [&](int y) {
        int n = 0;
        for (int x = 0; x < sz.width(); ++x) {
            QColor c = img.pixelColor(x, y);
            if (c.blue() > 120 && c.red() < 160) ++n;  // bond pixel (bluish, not white bg)
        }
        return n;
    };
    int bottom = -1;
    for (int y = sz.height() - 1; y >= 0; --y) { if (filled_width(y) > 0) { bottom = y; break; } }
    ASSERT_GT(bottom, 10);
    // A rounded hemisphere tapers to a point: the bottom row is a small fraction
    // of the body width a few rows up. A flat slice drops from full width to zero
    // in one row, so its bottom row is nearly the full body width. Measured:
    // rounded bottom/body ~= 8/38 ~ 0.2; flat slice ~= 34/36 ~ 0.94.
    const int body = filled_width(bottom - 6);
    ASSERT_GT(body, 0);
    EXPECT_LT(filled_width(bottom), body / 2)
        << "near cap is a flat slice, not a rounded hemisphere (bottom width="
        << filled_width(bottom) << " vs body=" << body << ")";
}





TEST(BondOrderRenderTest, DoubleBondShowsTwoSeparatedCylinders) {
    // A single double bond between two atoms, plus a neighbor so the offset plane
    // is in-screen. Camera looks down -Z; bond along X, offset along Y.
    const QSize sz(256, 256);
    RhiContext ctx(sz);
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    OEChem::OEAtomBase* h = mol.NewAtom(1);
    double pc[3] = {-0.6, 0, 0};
    double po[3] = {0.6, 0, 0};
    double ph[3] = {-1.2, 0.6, 0};  // neighbor in +y so offset is vertical on screen
    mol.SetCoords(c, pc); mol.SetCoords(o, po); mol.SetCoords(h, ph);
    mol.NewBond(c, o, 2);
    mol.NewBond(c, h, 1);
    const OEChem::OEMolBase& base = mol;

    BondData bonds = build_bond_data(&base, StickStyle{}.radius(0.08));
    AtomData none;
    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(none, bonds);
    Camera cam; cam.zoom_to(bonds.bounds);
    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());

    // Scan a vertical line through the bond center (x = width/2). The two parallel
    // cylinders appear as two filled bands separated by a background gap.
    const int x = sz.width() / 2;
    int bands = 0;
    bool prev_filled = false;
    for (int y = 0; y < sz.height(); ++y) {
        QColor c2 = img.pixelColor(x, y);
        const bool filled = (c2.red() + c2.green() + c2.blue()) > 60;  // not black bg
        if (filled && !prev_filled) ++bands;
        prev_filled = filled;
    }
    EXPECT_GE(bands, 2) << "double bond should show two separated cylinders (bands="
                        << bands << ")";
}

TEST(BondOrderRenderTest, AromaticBondShowsTwoSeparatedLines) {
    // An aromatic bond renders like a double bond: a symmetric pair of thin lines
    // straddling the axis (solid outer + dashed inner), so a scan perpendicular to
    // the bond crosses TWO filled bands with a background gap between them -- not
    // one fat single cylinder. Build a single horizontal aromatic bond (face-on)
    // with a neighbor so the offset plane is in-screen; offset is vertical.
    const QSize sz(256, 256);
    RhiContext ctx(sz);
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c1 = mol.NewAtom(6);
    OEChem::OEAtomBase* c2 = mol.NewAtom(6);
    OEChem::OEAtomBase* c3 = mol.NewAtom(6);  // neighbor -> in-plane offset (+y)
    double p1[3] = {-0.7, 0, 0}, p2[3] = {0.7, 0, 0}, p3[3] = {-1.4, 0.7, 0};
    mol.SetCoords(c1, p1); mol.SetCoords(c2, p2); mol.SetCoords(c3, p3);
    OEChem::OEBondBase* bd = mol.NewBond(c1, c2, 1);
    bd->SetAromatic(true);
    c1->SetAromatic(true); c2->SetAromatic(true);
    mol.NewBond(c1, c3, 1);
    const OEChem::OEMolBase& base = mol;

    BondData bonds = build_bond_data(&base, StickStyle{}.radius(0.10));
    AtomData none;
    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(none, bonds);
    Camera cam; cam.zoom_to(bonds.bounds);
    QImage img = ctx.render_frame(QColor(255, 255, 255), [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
    });
    ASSERT_FALSE(img.isNull());

    // Scan the central column (x = width/2, where the bond crosses) vertically and
    // count filled bands. The solid line + the dashed line (where a dash lands at
    // center) read as two separate bands; even if a dash gap falls on this column,
    // the next column over catches it -- so scan a few central columns and take the
    // max band count, which must be >= 2 for the straddling pair.
    auto bands_at = [&](int x) {
        int bands = 0; bool prev = false;
        for (int y = 0; y < sz.height(); ++y) {
            QColor c = img.pixelColor(x, y);
            const bool filled = c.red() < 220 || c.green() < 220 || c.blue() < 220;
            if (filled && !prev) ++bands;
            prev = filled;
        }
        return bands;
    };
    int max_bands = 0;
    for (int x = sz.width() / 2 - 8; x <= sz.width() / 2 + 8; ++x)
        max_bands = std::max(max_bands, bands_at(x));
    EXPECT_GE(max_bands, 2)
        << "aromatic bond should show two separated lines (solid + dashed), bands="
        << max_bands;
}

TEST(BondOrderRenderTest, DoubleBondNotMuchWiderThanSingle) {
    // A double bond renders as two thinner offset cylinders that together read as
    // ~one-bond-wide (3dmol look). The earlier bug spaced two FULL-radius cylinders
    // ~a half-bond-length apart, making the double bond ~3x the visual width of a
    // single bond. Render the SAME C-O bond as single vs double (default style),
    // measure the max filled height of the central column, and require the double
    // to be no more than 1.7x the single. (3x = the bug.)
    const QSize sz(256, 256);
    auto bond_height = [&](int order) {
        OEChem::OEGraphMol mol;
        OEChem::OEAtomBase* c = mol.NewAtom(6);
        OEChem::OEAtomBase* o = mol.NewAtom(8);
        OEChem::OEAtomBase* h = mol.NewAtom(1);  // neighbor -> in-screen offset (+y)
        double pc[3] = {-0.7, 0, 0}, po[3] = {0.7, 0, 0}, ph[3] = {-1.3, 0.7, 0};
        mol.SetCoords(c, pc); mol.SetCoords(o, po); mol.SetCoords(h, ph);
        mol.NewBond(c, o, order);
        mol.NewBond(c, h, 1);
        const OEChem::OEMolBase& base = mol;
        RhiContext ctx(sz);
        BondData bonds = build_bond_data(&base, StickStyle{});
        AtomData none;
        SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
        renderer.set_geometry(none, bonds);
        Camera cam; cam.zoom_to(bonds.bounds);
        QImage img = ctx.render_frame(QColor(255, 255, 255), [&](QRhiCommandBuffer* cb) {
            renderer.draw(cb, cam, sz);
        });
        EXPECT_FALSE(img.isNull());
        // Max filled height over the central columns (the bond runs horizontally;
        // the C=O occupies the left-center, so scan a band of columns and take the
        // tallest filled run to capture the full double-bond spread).
        int max_h = 0;
        for (int x = sz.width() / 4; x < sz.width() / 2; ++x) {
            int h = 0;
            for (int y = 0; y < sz.height(); ++y) {
                QColor cc = img.pixelColor(x, y);
                if (cc.red() < 220 || cc.green() < 220 || cc.blue() < 220) ++h;  // non-bg
            }
            max_h = std::max(max_h, h);
        }
        return max_h;
    };
    const int single_h = bond_height(1);
    const int double_h = bond_height(2);
    ASSERT_GT(single_h, 0);
    EXPECT_GT(double_h, single_h)  // a double IS a bit wider (two offset lines)
        << "double bond should be slightly wider than single";
    EXPECT_LT(double_h, single_h * 17 / 10)  // ...but NOT ~3x (the bug)
        << "double bond far too wide (double_h=" << double_h << " single_h=" << single_h << ")";
}

}  // namespace test
}  // namespace oe3dmol
