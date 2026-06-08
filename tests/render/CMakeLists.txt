add_executable(oe3dmol_render_tests
    test_offscreen_render.cpp
)
target_link_libraries(oe3dmol_render_tests
    PRIVATE oe3dmol_render GTest::gtest_main Qt6::Gui Qt6::GuiPrivate)
gtest_discover_tests(oe3dmol_render_tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
