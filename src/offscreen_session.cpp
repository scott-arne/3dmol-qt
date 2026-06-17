#include "oe3dmol/offscreen_session.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "oe3dmol/detail/validate_molecule.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/rhi_context.h"
#include "oe3dmol/scene_renderer.h"

#include <QDir>
#include <QQuaternion>
#include <rhi/qrhi.h>

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

void OffscreenSession::ensure_device() {
    if (ctx_) {
        return;
    }
    // RequireHardware (default): reject the Null backend so a GPU-less host fails
    // loudly instead of producing unrendered frames.
    ctx_ = std::make_unique<RhiContext>(size_);
    renderer_ = std::make_unique<SceneRenderer>(ctx_->rhi(), ctx_->render_pass_descriptor());
    // Push the current CPU geometry into the freshly built renderer (mirrors the
    // widget's initialize()). On a device-loss rebuild this re-uploads to the new
    // device; on first build it uploads the geometry add_model already built.
    renderer_->set_geometry(atoms_, bonds_);
}

QImage OffscreenSession::render_once() {
    if (models_.empty()) {
        throw RenderError("OffscreenSession: no models to render");
    }
    if (!scene_bounds_.valid()) {
        throw RenderError("OffscreenSession: models produced no drawable geometry");
    }
    ensure_device();
    const QSize sz = size_;
    return ctx_->render_frame(background_, [&](QRhiCommandBuffer* cb) {
        renderer_->draw(cb, camera_, sz);
    });
}

QImage OffscreenSession::render_frame() {
    try {
        return render_once();
    } catch (const DeviceLostError&) {
        // Auto-recover once: drop the lost device + renderer, rebuild on the next
        // render_once (ensure_device re-pushes the current geometry to the fresh
        // device), and retry. The camera state is owned by the session, so it
        // survives the rebuild and the animation continues seamlessly.
        renderer_.reset();  // destroy pipelines (they reference ctx_'s device) first
        ctx_.reset();       // then the device/context they depend on
        try {
            return render_once();
        } catch (const DeviceLostError& e) {
            throw RenderError(std::string("OffscreenSession: device lost and recovery "
                                          "failed: ") + e.what());
        }
    }
}

void OffscreenSession::save_frame(const std::string& path) {
    QImage img = render_frame();  // throws RenderError on render failure
    if (!img.save(QString::fromStdString(path), "PNG")) {
        throw RenderError("OffscreenSession: failed to write PNG to '" + path +
                          "' (check path, permissions, and that the file is not a "
                          "directory)");
    }
}

void OffscreenSession::save_turntable(const std::string& dir, int n_frames,
                                      const QVector3D& axis, float total_degrees) {
    if (n_frames <= 0) {
        throw RenderError("OffscreenSession::save_turntable: n_frames must be positive");
    }
    if (axis.isNull()) {  // zero-length axis -> orbit rotation is undefined
        throw RenderError("OffscreenSession::save_turntable: axis must be non-zero");
    }
    if (!std::isfinite(total_degrees)) {
        throw RenderError("OffscreenSession::save_turntable: total_degrees must be finite");
    }
    const QDir out(QString::fromStdString(dir));
    if (!out.exists()) {
        throw RenderError("OffscreenSession::save_turntable: directory does not exist: '" +
                          dir + "'");
    }

    // Zero-pad the frame index to fit the largest value; minimum width 4
    // (frame_0000.png), widening only for very large sequences.
    const int width =
        std::max(4, static_cast<int>(std::to_string(n_frames - 1).size()));
    const float step = total_degrees / static_cast<float>(n_frames);

    // Snapshot the camera so the session is left exactly as found (Camera is
    // copyable). Orbiting AFTER each save makes frame 0 the un-rotated view and
    // the final step complete the sweep without duplicating frame 0.
    const Camera saved = camera_;
    try {
        for (int i = 0; i < n_frames; ++i) {
            std::ostringstream name;
            name << "frame_" << std::setw(width) << std::setfill('0') << i << ".png";
            const QString path = out.filePath(QString::fromStdString(name.str()));
            save_frame(path.toStdString());  // reuses the full render + recovery path
            camera_.orbit(QQuaternion::fromAxisAndAngle(axis, step));
        }
    } catch (...) {
        camera_ = saved;  // restore even if a frame fails mid-sweep
        throw;
    }
    camera_ = saved;
}

}  // namespace oe3dmol
