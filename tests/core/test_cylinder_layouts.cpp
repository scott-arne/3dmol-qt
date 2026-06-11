#include <gtest/gtest.h>

#include "oe3dmol/cylinder_layouts.h"

namespace oe3dmol {
namespace test {

TEST(CylinderLayoutsTest, InstanceLayoutIsElevenFloats) {
    EXPECT_EQ(CylinderInstanceLayout::FLOATS_PER_INSTANCE, 11);
    EXPECT_EQ(CylinderInstanceLayout::START_OFFSET, 0);
    EXPECT_EQ(CylinderInstanceLayout::END_OFFSET, 3);
    EXPECT_EQ(CylinderInstanceLayout::RADIUS_OFFSET, 6);
    EXPECT_EQ(CylinderInstanceLayout::COLOR_OFFSET, 7);
}

TEST(CylinderLayoutsTest, BillboardLayoutIsTwoFloats) {
    EXPECT_EQ(CylinderBillboardLayout::FLOATS_PER_VERTEX, 2);
    EXPECT_EQ(CylinderBillboardLayout::ENDPOINT_SEL_OFFSET, 0);
    EXPECT_EQ(CylinderBillboardLayout::CORNER_SIGN_OFFSET, 1);
}

}  // namespace test
}  // namespace oe3dmol
