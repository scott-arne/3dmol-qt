#include <gtest/gtest.h>

#include "oe3dmol/stick_style.h"

namespace oe3dmol {
namespace test {

TEST(StickStyleTest, DefaultRadiusIs0_25) {
    StickStyle s;
    EXPECT_DOUBLE_EQ(s.radius(), 0.25);
}

TEST(StickStyleTest, DefaultColorSchemeIsJmol) {
    StickStyle s;
    EXPECT_EQ(s.color(), ColorScheme::Jmol);
}

TEST(StickStyleTest, FluentSettersChain) {
    StickStyle s;
    s.radius(0.4).color(ColorScheme::Default).opacity(0.5);
    EXPECT_DOUBLE_EQ(s.radius(), 0.4);
    EXPECT_EQ(s.color(), ColorScheme::Default);
    EXPECT_DOUBLE_EQ(s.opacity(), 0.5);
}

}  // namespace test
}  // namespace oe3dmol
