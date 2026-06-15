include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
# Keep googletest out of our install tree (it ships its own install() rules).
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_subdirectory(core)
add_subdirectory(geometry)
add_subdirectory(data)
add_subdirectory(widget)

if(OE3DMOL_BUILD_GPU_TESTS)
    add_subdirectory(render)
endif()
