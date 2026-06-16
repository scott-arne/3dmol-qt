#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "oe3dmol/data_errors.h"
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

TEST(StickStyleTest, SingleBondsDefaultsFalseAndIsChainable) {
    StickStyle s;
    EXPECT_FALSE(s.single_bonds());          // multiplicity/aromaticity ON by default
    StickStyle& ref = s.single_bonds(true);  // fluent setter returns *this
    EXPECT_TRUE(s.single_bonds());
    EXPECT_EQ(&ref, &s);
}

TEST(StickStyleTest, AromaticStyleDefaultsKekuleAndIsChainable) {
    StickStyle s;
    EXPECT_EQ(s.aromatic_style(), AromaticStyle::Kekule);  // default
    StickStyle& ref = s.aromatic_style(AromaticStyle::Delocalized);
    EXPECT_EQ(s.aromatic_style(), AromaticStyle::Delocalized);
    EXPECT_EQ(&ref, &s);
}

TEST(StickStyleValidation, RejectsNegativeAndNonFiniteRadius) {
    EXPECT_THROW(StickStyle{}.radius(-1.0), DataError);
    EXPECT_THROW(StickStyle{}.radius(std::nan("")), DataError);
    EXPECT_THROW(StickStyle{}.radius(std::numeric_limits<double>::infinity()), DataError);
}
TEST(StickStyleValidation, AcceptsZeroAndPositive) {
    EXPECT_NO_THROW(StickStyle{}.radius(0.0));
    EXPECT_NO_THROW(StickStyle{}.radius(0.4));
}

}  // namespace test
}  // namespace oe3dmol
