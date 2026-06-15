#include "oe3dmol/viewer.h"

#include "oe3dmol/camera.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/rhi_context.h"
#include "oe3dmol/scene_builder.h"
#include "oe3dmol/scene_renderer.h"

#include <rhi/qrhi.h>

namespace oe3dmol {

Viewer::Viewer(int width, int height) : size_(width, height) {
    if (width <= 0 || height <= 0) {
        throw RenderError("Viewer: width and height must be positive");
    }
}

Viewer::~Viewer() = default;

int Viewer::add_model(const OEChem::OEMolBase* mol) {
    models_.push_back(SceneModel{mol, {SphereStyle{}}});
    return static_cast<int>(models_.size()) - 1;
}

void Viewer::set_style(int model_index, const SphereStyle& style) {
    set_style(model_index, std::vector<StyleSpec>{style});
}

void Viewer::set_style(int model_index, const std::vector<StyleSpec>& styles) {
    if (model_index < 0 || model_index >= static_cast<int>(models_.size())) {
        throw RenderError("Viewer: set_style model index out of range");
    }
    models_[static_cast<std::size_t>(model_index)].styles = styles;
}

void Viewer::zoom_to() {
    // Camera is fit at render time from the combined scene bounds.
}

QImage Viewer::render() {
    if (models_.empty()) {
        throw RenderError("Viewer: no models to render");
    }

    AtomData atoms;
    BondData bonds;
    BoundingBox scene_bounds;
    build_scene(models_, atoms, bonds, scene_bounds);

    Camera cam;
    // render() already threw above if models_ is empty, so scene_bounds is always
    // valid here; the old atoms.bounds fallback was unreachable.
    cam.zoom_to(scene_bounds);

    RhiContext ctx(size_);
    SceneRenderer renderer(ctx.rhi(), ctx.render_pass_descriptor());
    renderer.set_geometry(atoms, bonds);

    const QSize sz = size_;
    return ctx.render_frame(background_, [&](QRhiCommandBuffer* cb) {
        renderer.draw(cb, cam, sz);
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
