add_executable(render_molecule render_molecule.cpp)
target_link_libraries(render_molecule
    PRIVATE oe3dmol_render Qt6::Gui OpenEye::OEChem)
