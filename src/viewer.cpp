#include "oe3dmol/viewer.h"

#include "oe3dmol/camera.h"
#include "oe3dmol/mesh_buffer.h"
#include "oe3dmol/mol_adapter.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/rhi_context.h"
#include "oe3dmol/sphere_imposter_builder.h"
#include "oe3dmol/sphere_imposter_pipeline.h"

#include <rhi/qrhi.h>

namespace oe3dmol {

Viewer::Viewer(int width, int height) : size_(width, height) {
    if (width <= 0 || height <= 0) {
        throw RenderError("Viewer: width and height must be positive");
    }
}

Viewer::~Viewer() = default;

int Viewer::add_model(const OEChem::OEMolBase* mol) {
    models_.push_back(Model{mol, SphereStyle{}});
    return static_cast<int>(models_.size()) - 1;
}

void Viewer::set_style(int model_index, const SphereStyle& style) {
    if (model_index < 0 || model_index >= static_cast<int>(models_.size())) {
        throw RenderError("Viewer: set_style model index out of range");
    }
    models_[static_cast<std::size_t>(model_index)].style = style;
}

void Viewer::zoom_to() {
    // Camera is fit at render time from combined_ bounds; this method exists
    // so callers can mirror the 3dmol.js flow (addModel/setStyle/zoomTo/render).
}

QImage Viewer::render() {
    if (models_.empty()) {
        throw RenderError("Viewer: no models to render");
    }

    combined_ = AtomData{};
    for (const Model& m : models_) {
        AtomData d = build_atom_data(m.mol, m.style);
        for (std::size_t i = 0; i < d.size(); ++i) {
            combined_.add(d.positions[i], d.radii[i], d.colors[i], d.source_idx[i]);
        }
    }

    MeshBuffer mesh = build_sphere_imposters(combined_);

    Camera cam;
    cam.zoom_to(combined_.bounds);

    RhiContext ctx(size_);
    SphereImposterPipeline pipeline(ctx.rhi(), ctx.render_pass_descriptor());

    ImposterUniforms u;
    u.view = cam.view_matrix();
    u.projection = cam.projection_matrix(
        static_cast<float>(size_.width()) / static_cast<float>(size_.height()));
    u.model_view = u.view;

    const QSize sz = size_;
    return ctx.render_frame(background_, [&](QRhiCommandBuffer* cb) {
        QRhiResourceUpdateBatch* batch = ctx.rhi()->nextResourceUpdateBatch();
        pipeline.upload(mesh, batch);
        pipeline.set_uniforms(u, batch);
        cb->resourceUpdate(batch);
        pipeline.draw(cb, sz);
    });
}

bool Viewer::render_to_png(const std::string& path) {
    QImage img = render();
    if (img.isNull()) {
        return false;
    }
    return img.save(QString::fromStdString(path), "PNG");
}

}  // namespace oe3dmol
