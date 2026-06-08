#include <gtest/gtest.h>

#include "oe3dmol/version.h"

#include <string>

namespace oe3dmol {
namespace test {

TEST(VersionTest, ReturnsExpectedString) {
    EXPECT_EQ(std::string(version_string()), "0.1.0");
}

}  // namespace test
}  // namespace oe3dmol
