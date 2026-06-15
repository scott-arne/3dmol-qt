add_executable(oe3dmol_data_tests
    test_mol_adapter.cpp
    test_bond_adapter.cpp
    test_scene_builder.cpp
)
target_link_libraries(oe3dmol_data_tests PRIVATE oe3dmol_data GTest::gtest_main)
gtest_discover_tests(oe3dmol_data_tests)
