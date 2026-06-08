#include <gtest/gtest.h>

#include "oe3dmol/vdw_radii.h"

namespace oe3dmol {
namespace test {

TEST(VdwRadiiTest, CarbonIs1_7) {
    EXPECT_DOUBLE_EQ(vdw_radius("C"), 1.7);
}

TEST(VdwRadiiTest, HydrogenIs1_2) {
    EXPECT_DOUBLE_EQ(vdw_radius("H"), 1.2);
}

TEST(VdwRadiiTest, OxygenIs1_52) {
    EXPECT_DOUBLE_EQ(vdw_radius("O"), 1.52);
}

TEST(VdwRadiiTest, UnknownElementIsDefault1_5) {
    EXPECT_DOUBLE_EQ(vdw_radius("Xx"), DEFAULT_SPHERE_RADIUS);
}

}  // namespace test
}  // namespace oe3dmol
