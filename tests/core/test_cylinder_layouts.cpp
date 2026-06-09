#include <gtest/gtest.h>

#include "oe3dmol/cylinder_layouts.h"

namespace oe3dmol {
namespace test {

TEST(CylinderLayoutsTest, VertexLayoutIsSixFloats) {
    EXPECT_EQ(CylinderVertexLayout::FLOATS_PER_VERTEX, 6);
    EXPECT_EQ(CylinderVertexLayout::POSITION_OFFSET, 0);
    EXPECT_EQ(CylinderVertexLayout::NORMAL_OFFSET, 3);
}

TEST(CylinderLayoutsTest, InstanceLayoutIsElevenFloats) {
    EXPECT_EQ(CylinderInstanceLayout::FLOATS_PER_INSTANCE, 11);
    EXPECT_EQ(CylinderInstanceLayout::START_OFFSET, 0);
    EXPECT_EQ(CylinderInstanceLayout::END_OFFSET, 3);
    EXPECT_EQ(CylinderInstanceLayout::RADIUS_OFFSET, 6);
    EXPECT_EQ(CylinderInstanceLayout::COLOR_OFFSET, 7);
}

}  // namespace test
}  // namespace oe3dmol
