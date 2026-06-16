// Single global helper to register the embedded shader .qsb resources. The .qsb
// files live in a generated translation unit whose static initializer is dropped
// by the linker when oe3dmol_render is consumed as a static library, so reference
// it explicitly. Q_INIT_RESOURCE expands to a declaration of the global
// qInitResources_shaders symbol, so it MUST live at true global scope (outside
// any namespace) for the symbol to resolve.

#include <QtResource>

#include "oe3dmol/detail/shader_resources.h"

void oe3dmol_ensure_shader_resources() {
    Q_INIT_RESOURCE(shaders);
}
