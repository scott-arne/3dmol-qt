#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "oe3dmol/data_errors.h"
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

TEST(SphereStyleValidation, RejectsNegativeRadius) {
    EXPECT_THROW(SphereStyle{}.radius(-1.0), DataError);
}
TEST(SphereStyleValidation, RejectsNonFiniteRadius) {
    EXPECT_THROW(SphereStyle{}.radius(std::nan("")), DataError);
    EXPECT_THROW(SphereStyle{}.radius(std::numeric_limits<double>::infinity()), DataError);
}
TEST(SphereStyleValidation, RejectsNegativeOrNonFiniteScale) {
    EXPECT_THROW(SphereStyle{}.scale(-0.5), DataError);
    EXPECT_THROW(SphereStyle{}.scale(std::nan("")), DataError);
}
TEST(SphereStyleValidation, AcceptsZeroAndPositive) {
    EXPECT_NO_THROW(SphereStyle{}.radius(0.0).scale(0.0));
    EXPECT_NO_THROW(SphereStyle{}.radius(1.5).scale(2.0));
}

}  // namespace test
}  // namespace oe3dmol
