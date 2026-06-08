# Compiles GLSL shaders to .qsb using Qt's qsb tool.
# Usage: oe3dmol_compile_shaders(<out_var> shader1.vert shader2.frag ...)

find_program(QSB_EXECUTABLE NAMES qsb qsb-qt6
    HINTS "${Qt6_DIR}/../../../bin" "$ENV{QT_ROOT_DIR}/bin")
if(NOT QSB_EXECUTABLE)
    message(FATAL_ERROR "qsb tool not found. Install Qt with shader tools (brew install qt).")
endif()

function(oe3dmol_compile_shaders OUT_VAR)
    set(_outputs "")
    foreach(_src ${ARGN})
        get_filename_component(_name "${_src}" NAME)
        set(_out "${CMAKE_CURRENT_BINARY_DIR}/${_name}.qsb")
        add_custom_command(
            OUTPUT "${_out}"
            COMMAND "${QSB_EXECUTABLE}" --glsl "100es,120,150,440" --hlsl 50 --msl 12
                    -o "${_out}" "${CMAKE_CURRENT_SOURCE_DIR}/${_src}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_src}"
            COMMENT "qsb ${_name}"
            VERBATIM
        )
        list(APPEND _outputs "${_out}")
    endforeach()
    set(${OUT_VAR} "${_outputs}" PARENT_SCOPE)
endfunction()
