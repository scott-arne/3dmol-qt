cmake_minimum_required(VERSION 3.17)

if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

project(3dmol-qt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OE3DMOL_BUILD_TESTS "Build tests" ON)
option(OE3DMOL_BUILD_GPU_TESTS "Build tests that require a live RHI device" ON)
option(OE3DMOL_BUILD_EXAMPLES "Build examples" ON)
option(OE3DMOL_UNIVERSAL2 "Build universal2 binary for macOS" OFF)
option(OE3DMOL_WERROR "Treat compiler warnings as errors" OFF)
option(OE3DMOL_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

if(APPLE AND OE3DMOL_UNIVERSAL2)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures" FORCE)
elseif(APPLE)
    unset(CMAKE_OSX_ARCHITECTURES CACHE)
endif()

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

include(FetchContent)

# OpenEye via cmake-openeye
FetchContent_Declare(
    cmake_openeye
    GIT_REPOSITORY https://github.com/scott-arne/cmake-openeye.git
    GIT_TAG v1.2.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cmake_openeye)
list(APPEND CMAKE_MODULE_PATH "${cmake_openeye_SOURCE_DIR}")
find_package(OpenEye REQUIRED)

# Qt6 (Core + Gui everywhere; GuiPrivate only used by the render target)
find_package(Qt6 REQUIRED COMPONENTS Core Gui GuiPrivate Widgets)
if(Qt6_VERSION VERSION_LESS 6.6)
    message(FATAL_ERROR "Qt 6.6+ is required for a stable QRhi API (found ${Qt6_VERSION}).")
endif()

# Apply the project's warning level (and optional Werror) to one of our own
# targets. Applied PRIVATE so flags never propagate to consumers and do not
# enter the install/export interface. Qt and OpenEye come in as imported
# targets, whose interface includes CMake treats as -isystem by default, so
# third-party header warnings stay quiet. (Sanitizers are applied globally
# below, not here -- they must instrument the linking executables too.)
function(oe3dmol_set_warnings tgt)
    target_compile_options(${tgt} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<AND:$<BOOL:${OE3DMOL_WERROR}>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-Werror>
        $<$<AND:$<BOOL:${OE3DMOL_WERROR}>,$<CXX_COMPILER_ID:MSVC>>:/WX>
    )
endfunction()

# Sanitizers must instrument the whole program -- the libraries AND the
# executables that link them (tests, examples) -- or the runtime is not pulled
# in and instrumentation is unreliable. Apply globally (before any target is
# defined) so every translation unit and link line is covered.
if(OE3DMOL_SANITIZE AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Targets are added by subsequent tasks.

# ---- oe3dmol_core ----
add_library(oe3dmol_core
    src/version.cpp
    src/color_schemes.cpp
    src/vdw_radii.cpp
    src/style.cpp
    src/bounding_box.cpp
    src/camera.cpp
    src/arcball.cpp
)
set_target_properties(oe3dmol_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(oe3dmol_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(oe3dmol_core PUBLIC Qt6::Core Qt6::Gui)
oe3dmol_set_warnings(oe3dmol_core)

# ---- oe3dmol_geometry ----
add_library(oe3dmol_geometry
    src/sphere_imposter_builder.cpp
    src/cylinder_builder.cpp
)
set_target_properties(oe3dmol_geometry PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(oe3dmol_geometry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(oe3dmol_geometry PUBLIC oe3dmol_core)
oe3dmol_set_warnings(oe3dmol_geometry)

# ---- oe3dmol_data ----
add_library(oe3dmol_data
    src/mol_adapter.cpp
    src/bond_adapter.cpp
    src/scene_builder.cpp
)
set_target_properties(oe3dmol_data PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(oe3dmol_data
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(oe3dmol_data
    PUBLIC
        oe3dmol_core
        OpenEye::OEChem
        OpenEye::OEGrid
        OpenEye::OESystem
)
oe3dmol_set_warnings(oe3dmol_data)

# ---- oe3dmol_render ----
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compile_shaders.cmake)
oe3dmol_compile_shaders(OE3DMOL_SHADER_QSB
    shaders/sphereimposter.vert
    shaders/sphereimposter.frag
    shaders/cylinder.vert
    shaders/cylinder.frag
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shaders.qrc
    ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc COPYONLY)
qt_add_resources(OE3DMOL_SHADER_RES
    ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc)

add_library(oe3dmol_render
    src/rhi_context.cpp
    src/imposter_uniforms.cpp
    src/sphere_imposter_pipeline.cpp
    src/cylinder_pipeline.cpp
    src/viewer.cpp
    src/scene_renderer.cpp
    ${OE3DMOL_SHADER_QSB}
    ${OE3DMOL_SHADER_RES}
)
set_target_properties(oe3dmol_render PROPERTIES POSITION_INDEPENDENT_CODE ON AUTORCC OFF)
target_include_directories(oe3dmol_render
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(oe3dmol_render
    PUBLIC
        oe3dmol_core
        oe3dmol_geometry
        oe3dmol_data
        Qt6::Gui
        Qt6::GuiPrivate
)
oe3dmol_set_warnings(oe3dmol_render)

# ---- oe3dmol_widget (interactive QRhiWidget; the only QtWidgets consumer) ----
# The Q_OBJECT header lives under include/oe3dmol; list it explicitly so AUTOMOC
# moc's it (AUTOMOC does not discover headers via a subdirectory-prefixed include).
add_library(oe3dmol_widget
    src/molecule_viewer_widget.cpp
    include/oe3dmol/molecule_viewer_widget.h
)
set_target_properties(oe3dmol_widget PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(oe3dmol_widget
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(oe3dmol_widget
    PUBLIC
        oe3dmol_core
        oe3dmol_render
        oe3dmol_data
        Qt6::Widgets
        Qt6::GuiPrivate
)
oe3dmol_set_warnings(oe3dmol_widget)

if(OE3DMOL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(OE3DMOL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ---- Install / export ----
include(CMakePackageConfigHelpers)

install(TARGETS oe3dmol_core oe3dmol_geometry oe3dmol_data oe3dmol_render oe3dmol_widget
    EXPORT oe3dmol-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/oe3dmol DESTINATION include
    PATTERN "detail" EXCLUDE)
install(EXPORT oe3dmol-targets
    FILE OE3DMOLTargets.cmake
    NAMESPACE OE3DMOL::
    DESTINATION lib/cmake/oe3dmol
)

# Bundle the FindOpenEye module so the installed config can locate OpenEye.
install(FILES "${cmake_openeye_SOURCE_DIR}/FindOpenEye.cmake"
    DESTINATION lib/cmake/oe3dmol
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/OE3DMOLConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/OE3DMOLConfig.cmake"
    INSTALL_DESTINATION lib/cmake/oe3dmol
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/OE3DMOLConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OE3DMOLConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OE3DMOLConfigVersion.cmake"
    DESTINATION lib/cmake/oe3dmol
)
