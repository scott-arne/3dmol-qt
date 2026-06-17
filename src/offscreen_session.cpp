#include "oe3dmol/offscreen_session.h"

#include "oe3dmol/detail/validate_molecule.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/rhi_context.h"
#include "oe3dmol/scene_renderer.h"

namespace oe3dmol {

OffscreenSession::OffscreenSession(int width, int height) : size_(width, height) {
    if (width <= 0 || height <= 0) {
        throw RenderError("OffscreenSession: width and height must be positive");
    }
}

OffscreenSession::~OffscreenSession() = default;

int OffscreenSession::add_model(const OEChem::OEMolBase* mol) {
    detail::require_renderable_molecule(mol, "OffscreenSession::add_model");
    models_.push_back(SceneModel{mol, {SphereStyle{}}});
    rebuild_geometry();
    return static_cast<int>(models_.size()) - 1;
}

void OffscreenSession::set_style(int model_index, const SphereStyle& style) {
    set_style(model_index, std::vector<StyleSpec>{style});
}

void OffscreenSession::set_style(int model_index, const std::vector<StyleSpec>& styles) {
    if (model_index < 0 || model_index >= static_cast<int>(models_.size())) {
        throw RenderError("OffscreenSession: set_style model index out of range");
    }
    models_[static_cast<std::size_t>(model_index)].styles = styles;
    rebuild_geometry();
}

void OffscreenSession::set_background_color(const QColor& color) {
    background_ = color;
}

void OffscreenSession::reframe() {
    if (scene_bounds_.valid()) {
        camera_.zoom_to(scene_bounds_);
        camera_initialized_ = true;
    }
}

void OffscreenSession::rebuild_geometry() {
    // Build CPU geometry eagerly on every mutation (mirrors MoleculeViewerWidget).
    // The GPU upload stays dirty-tracked by SceneRenderer; we only push into it
    // when it already exists (otherwise ensure_device() pushes on first build).
    build_scene(models_, atoms_, bonds_, scene_bounds_);
    if (scene_bounds_.valid() && !camera_initialized_) {
        camera_.zoom_to(scene_bounds_);
        camera_initialized_ = true;
    }
    if (renderer_) {
        renderer_->set_geometry(atoms_, bonds_);
    }
}

// --- render path: implemented in Task 3 ---
void OffscreenSession::ensure_device() {}
QImage OffscreenSession::render_once() { return QImage(); }
QImage OffscreenSession::render_frame() { return QImage(); }
void OffscreenSession::save_frame(const std::string&) {}

}  // namespace oe3dmol
