#include <gtest/gtest.h>

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
    MeshBuffer unit = build_unit_cylinder(16);
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
        pipeline.upload(unit, inst, batch);
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
    MeshBuffer unit = build_unit_cylinder(16);
    std::vector<float> inst;  // empty
    CylinderPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());
    ImposterUniforms u;
    QImage img = ctx.render_frame(QColor(0, 0, 0), [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* batch = ctx.rhi()->nextResourceUpdateBatch();
        pipeline.upload(unit, inst, batch);
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
    MeshBuffer unit = build_unit_cylinder(16);
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
        pipeline.upload(unit, inst, batch);
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

}  // namespace test
}  // namespace oe3dmol
