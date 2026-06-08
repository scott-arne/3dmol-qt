#include <gtest/gtest.h>

#include "oe3dmol/rhi_context.h"
#include "oe3dmol/render_errors.h"

#include <QGuiApplication>
#include <QImage>

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

}  // namespace test
}  // namespace oe3dmol
