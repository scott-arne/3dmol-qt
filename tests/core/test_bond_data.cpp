#include <gtest/gtest.h>

#include "oe3dmol/bond_data.h"

namespace oe3dmol {
namespace test {

TEST(BondDataTest, EmptyHasZeroSize) {
    BondData b;
    EXPECT_EQ(b.size(), 0u);
    EXPECT_FALSE(b.bounds.valid());
}

TEST(BondDataTest, AddAccumulatesEntriesAndBounds) {
    BondData b;
    b.add(QVector3D(0, 0, 0), QVector3D(2, 0, 0), 0.25f, Color(255, 0, 0), 0u);
    ASSERT_EQ(b.size(), 1u);
    EXPECT_FLOAT_EQ(b.starts[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(b.ends[0].x(), 2.0f);
    EXPECT_FLOAT_EQ(b.radii[0], 0.25f);
    EXPECT_EQ(b.source_bond_idx[0], 0u);
    EXPECT_TRUE(b.bounds.valid());
    EXPECT_NEAR(b.bounds.center().x(), 1.0f, 1e-5f);
}

}  // namespace test
}  // namespace oe3dmol
