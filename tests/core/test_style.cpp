#include <gtest/gtest.h>

#include "oe3dmol/style.h"

namespace oe3dmol {
namespace test {

TEST(StyleTest, DefaultUsesVdwRadius) {
    SphereStyle s;
    EXPECT_DOUBLE_EQ(resolve_radius(s, "C"), 1.7);
}

TEST(StyleTest, ExplicitRadiusOverridesVdw) {
    SphereStyle s;
    s.radius(0.5);
    EXPECT_DOUBLE_EQ(resolve_radius(s, "C"), 0.5);
}

TEST(StyleTest, ScaleMultipliesVdwRadius) {
    SphereStyle s;
    s.scale(2.0);
    EXPECT_DOUBLE_EQ(resolve_radius(s, "C"), 3.4);  // 1.7 * 2.0
}

TEST(StyleTest, ScaleAppliesAfterExplicitRadius) {
    SphereStyle s;
    s.radius(1.0).scale(3.0);
    EXPECT_DOUBLE_EQ(resolve_radius(s, "C"), 3.0);
}

TEST(StyleTest, UnknownElementUsesDefaultRadius) {
    SphereStyle s;
    EXPECT_DOUBLE_EQ(resolve_radius(s, "Xx"), 1.5);
}

}  // namespace test
}  // namespace oe3dmol
