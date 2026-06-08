#include <gtest/gtest.h>

#include "oe3dmol/color_schemes.h"

namespace oe3dmol {
namespace test {

TEST(ColorSchemesTest, JmolCarbonIsGrey909090) {
    Color c = element_color(ColorScheme::Jmol, "C");
    EXPECT_EQ(c.red(), 0x90);
    EXPECT_EQ(c.green(), 0x90);
    EXPECT_EQ(c.blue(), 0x90);
}

TEST(ColorSchemesTest, JmolOxygenIsRedff0d0d) {
    Color c = element_color(ColorScheme::Jmol, "O");
    EXPECT_EQ(c.red(), 0xff);
    EXPECT_EQ(c.green(), 0x0d);
    EXPECT_EQ(c.blue(), 0x0d);
}

TEST(ColorSchemesTest, JmolNitrogenIsBlue3050f8) {
    Color c = element_color(ColorScheme::Jmol, "N");
    EXPECT_EQ(c.red(), 0x30);
    EXPECT_EQ(c.green(), 0x50);
    EXPECT_EQ(c.blue(), 0xf8);
}

TEST(ColorSchemesTest, UnknownElementFallsBackToDefaultff1493) {
    Color c = element_color(ColorScheme::Jmol, "Xx");
    EXPECT_EQ(c.red(), 0xff);
    EXPECT_EQ(c.green(), 0x14);
    EXPECT_EQ(c.blue(), 0x93);
}

TEST(ColorSchemesTest, TwoLetterElementCaseInsensitive) {
    Color a = element_color(ColorScheme::Jmol, "FE");
    Color b = element_color(ColorScheme::Jmol, "Fe");
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.red(), 0xe0);
    EXPECT_EQ(a.green(), 0x66);
    EXPECT_EQ(a.blue(), 0x33);
}

}  // namespace test
}  // namespace oe3dmol
