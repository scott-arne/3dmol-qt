add_executable(oe3dmol_core_tests
    test_version.cpp
    test_color_schemes.cpp
)
target_link_libraries(oe3dmol_core_tests PRIVATE oe3dmol_core GTest::gtest_main)
gtest_discover_tests(oe3dmol_core_tests)
