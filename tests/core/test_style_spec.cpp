#include <gtest/gtest.h>

#include "oe3dmol/style_spec.h"

#include <vector>

namespace oe3dmol {
namespace test {

TEST(StyleSpecTest, HoldsSphereStyle) {
    StyleSpec s = SphereStyle{}.scale(0.3);
    ASSERT_TRUE(std::holds_alternative<SphereStyle>(s));
    EXPECT_TRUE(std::get<SphereStyle>(s).scale().has_value());
}

TEST(StyleSpecTest, HoldsStickStyle) {
    StyleSpec s = StickStyle{}.radius(0.3);
    ASSERT_TRUE(std::holds_alternative<StickStyle>(s));
    EXPECT_DOUBLE_EQ(std::get<StickStyle>(s).radius(), 0.3);
}

TEST(StyleSpecTest, VectorOfMixedStyles) {
    std::vector<StyleSpec> styles = {SphereStyle{}.scale(0.3), StickStyle{}};
    EXPECT_EQ(styles.size(), 2u);
    EXPECT_TRUE(std::holds_alternative<SphereStyle>(styles[0]));
    EXPECT_TRUE(std::holds_alternative<StickStyle>(styles[1]));
}

}  // namespace test
}  // namespace oe3dmol
