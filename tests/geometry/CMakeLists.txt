add_executable(oe3dmol_geometry_tests
    test_sphere_imposter_builder.cpp
    test_cylinder_builder.cpp
)
target_link_libraries(oe3dmol_geometry_tests PRIVATE oe3dmol_geometry GTest::gtest_main)
gtest_discover_tests(oe3dmol_geometry_tests)
