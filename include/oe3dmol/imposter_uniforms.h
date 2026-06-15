#pragma once

/// \file imposter_uniforms.h
/// \brief Shared per-frame UBO struct + std140 packer for the sphere and
///        cylinder imposter pipelines (identical lighting/fog/camera layout).

#include <QMatrix4x4>
#include <QVector3D>
#include <QtGlobal>

namespace oe3dmol {

/// \brief Per-frame uniforms passed to the imposter shaders (std140 layout).
struct ImposterUniforms {
    QMatrix4x4 model_view;
    QMatrix4x4 projection;
    QMatrix4x4 view;
    /// Maps the OpenGL-convention clip space (z in [-1,1], from
    /// QMatrix4x4::perspective) into the active backend's clip volume
    /// (z in [0,1] for Metal/D3D/Vulkan). Apply to gl_Position ONLY; the depth
    /// math keeps using the uncorrected projection. Defaults to identity so
    /// code that sets matrices by hand is unaffected until it opts in.
    QMatrix4x4 clip_correction;
    QVector3D light_color = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D light_direction = QVector3D(0.0f, 0.0f, 1.0f);
    QVector3D fog_color = QVector3D(1.0f, 1.0f, 1.0f);
    float fog_near = 1.0f;
    float fog_far = 1.0f;  // fog_near == fog_far disables fog
    float opacity = 1.0f;
};

/// \brief std140 size of the imposter UBO: 4 mat4 (256) + 3 vec4 (48) + 4 floats
/// (16) = 320 bytes. Must match the `std140` block in sphereimposter.vert/.frag
/// and cylinder.vert/.frag, and the float count written by pack_imposter_uniforms.
constexpr quint32 IMPOSTER_UBO_SIZE = 320;

/// \brief Pack `u` into `out` in std140 order. `out` must point to at least
/// IMPOSTER_UBO_SIZE/sizeof(float) floats.
void pack_imposter_uniforms(const ImposterUniforms& u, float* out);

}  // namespace oe3dmol
