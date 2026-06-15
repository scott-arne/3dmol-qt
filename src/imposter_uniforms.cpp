#include "oe3dmol/imposter_uniforms.h"

namespace oe3dmol {

// IMPOSTER_UBO_SIZE, PACKED_FLOATS, the std140 struct fields, and the put_*
// sequence below must all be bumped together when a uniform is added. This
// assert is a tripwire for the common slip of editing one of the two size
// constants alone; it cannot detect a new field that updates neither, so keep
// the four in sync by hand.
static constexpr int PACKED_FLOATS = 4 * 16 + 3 * 4 + 4;  // mats + vec4s + scalars
static_assert(IMPOSTER_UBO_SIZE == PACKED_FLOATS * sizeof(float),
              "IMPOSTER_UBO_SIZE must match PACKED_FLOATS (the std140 float count)");

void pack_imposter_uniforms(const ImposterUniforms& u, float* out) {
    int o = 0;
    auto put_mat = [&](const QMatrix4x4& m) {
        const float* d = m.constData();  // column-major, matches std140 mat4
        for (int i = 0; i < 16; ++i) out[o++] = d[i];
    };
    put_mat(u.model_view);
    put_mat(u.projection);
    put_mat(u.view);
    put_mat(u.clip_correction);
    auto put_vec4 = [&](const QVector3D& v) {
        out[o++] = v.x();
        out[o++] = v.y();
        out[o++] = v.z();
        out[o++] = 0.0f;
    };
    put_vec4(u.light_color);
    put_vec4(u.light_direction);
    put_vec4(u.fog_color);
    out[o++] = u.fog_near;
    out[o++] = u.fog_far;
    out[o++] = u.opacity;
    out[o++] = 0.0f;
}

}  // namespace oe3dmol
