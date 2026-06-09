add_executable(oe3dmol_core_tests
    test_version.cpp
    test_color_schemes.cpp
    test_vdw_radii.cpp
    test_style.cpp
    test_bounding_box.cpp
    test_camera.cpp
    test_camera_orbit.cpp
    test_stick_style.cpp
    test_style_spec.cpp
    test_bond_data.cpp
    test_cylinder_layouts.cpp
)
target_link_libraries(oe3dmol_core_tests PRIVATE oe3dmol_core GTest::gtest_main)
gtest_discover_tests(oe3dmol_core_tests)
