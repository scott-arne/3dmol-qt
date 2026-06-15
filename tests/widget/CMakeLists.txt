add_executable(oe3dmol_widget_tests
    test_molecule_viewer_widget.cpp
)
target_link_libraries(oe3dmol_widget_tests
    PRIVATE oe3dmol_widget oe3dmol_data GTest::gtest_main
            Qt6::Widgets Qt6::Gui OpenEye::OEChem)
oe3dmol_set_warnings(oe3dmol_widget_tests)
# Widget interaction runs entirely in the event handlers (no RHI init), so the
# offscreen platform suffices -- no GPU device required.
gtest_discover_tests(oe3dmol_widget_tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
