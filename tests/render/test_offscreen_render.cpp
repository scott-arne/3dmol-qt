#include <gtest/gtest.h>

#include "oe3dmol/rhi_context.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/sphere_imposter_pipeline.h"
#include "oe3dmol/sphere_imposter_builder.h"
#include "oe3dmol/camera.h"
#include "oe3dmol/cylinder_pipeline.h"
#include "oe3dmol/cylinder_builder.h"
#include "oe3dmol/bond_data.h"

#include <QGuiApplication>
#include <QImage>

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

}  // namespace test
}  // namespace oe3dmol
