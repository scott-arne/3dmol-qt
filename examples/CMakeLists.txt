add_executable(render_molecule render_molecule.cpp)
target_link_libraries(render_molecule
    PRIVATE oe3dmol_render Qt6::Gui OpenEye::OEChem)

add_executable(view_molecule view_molecule.cpp)
target_link_libraries(view_molecule
    PRIVATE oe3dmol_widget Qt6::Widgets OpenEye::OEChem)
